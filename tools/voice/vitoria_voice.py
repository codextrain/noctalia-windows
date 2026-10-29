"""Offline PC shutdown commands. Audio is streamed in RAM, never saved or uploaded."""
import argparse, ctypes, json, os, queue, re, sys, time, unicodedata, uuid
from pathlib import Path

NUMBERS={'um':1,'uma':1,'dois':2,'duas':2,'tres':3,'quatro':4,'cinco':5,'seis':6,'sete':7,'oito':8,'nove':9,'dez':10,'onze':11,'doze':12,'treze':13,'quatorze':14,'catorze':14,'quinze':15,'dezesseis':16,'dezessete':17,'dezoito':18,'dezenove':19,'vinte':20,'trinta':30,'quarenta':40,'cinquenta':50,'sessenta':60,'noventa':90}
def normalize(text):
    return ' '.join(''.join(c for c in unicodedata.normalize('NFD',text.lower()) if unicodedata.category(c)!='Mn').replace(',',' ').split())
def number(text):
    if text.isdecimal(): return int(text)
    parts=text.split(' e ')
    if len(parts)>2 or any(x not in NUMBERS for x in parts): return None
    if len(parts)==2 and (NUMBERS[parts[0]]<20 or NUMBERS[parts[1]]>9): return None
    return sum(NUMBERS[x] for x in parts)
def parse(text):
    text=normalize(text)
    match=re.fullmatch(r'(?:pc|p c|pe ce) (.+)',text)
    if not match:return None
    body=match[1]
    if body == 'cancelar':return {'action':'cancel'}
    if body in ['desativar voz','desativar comandos de voz','desativar os comandos de voz']:return {'action':'disable'}
    match=re.fullmatch(r'desligar em (.+?) (minuto|minutos|hora|horas)',body)
    if not match:return None
    n=number(match[1])
    if n is None:return None
    minutes=n*(60 if match[2].startswith('hora') else 1)
    return {'action':'schedule','minutes':minutes} if 1<=minutes<=1440 else None

def spoken(n,feminine=False):
    words={v:k for k,v in NUMBERS.items()}
    # Reverse lookup otherwise picks the last aliases: uma/duas, wrong for minutos.
    words.update({1:'um',2:'dois',14:'quatorze'})
    text=words[n] if n in words else words[n//10*10]+' e '+words[n%10]
    if feminine:
        if text.endswith('um'):text=text[:-2]+'uma'
        elif text.endswith('dois'):text=text[:-4]+'duas'
    return text
def grammar():
    phrases=[]
    for wake in ['pê cê','p c']:
        phrases += [wake+' cancelar',wake+' desativar comandos de voz']
        phrases += [f'{wake} desligar em {spoken(n)} '+('minuto' if n==1 else 'minutos') for n in range(1,61)]
        phrases += [f'{wake} desligar em {spoken(n,True)} '+('hora' if n==1 else 'horas') for n in range(1,25)]
    return phrases+['[unk]']

def write_state(path,value):
    # Readers can briefly hold the destination open on Windows. Keep the previous
    # complete JSON and retry on the next heartbeat instead of terminating voice.
    tmp=path.with_name(path.name+'.'+uuid.uuid4().hex+'.tmp')
    try:
        path.parent.mkdir(parents=True,exist_ok=True)
        tmp.write_text(json.dumps(value,ensure_ascii=False),encoding='utf-8')
        for attempt in range(5):
            try:
                os.replace(tmp,path)
                return True
            except PermissionError:
                if attempt<4:time.sleep(.04)
        return False
    except OSError:
        return False
    finally:
        try:tmp.unlink(missing_ok=True)
        except OSError:pass


def enabled(config):
    try:return json.loads(config.read_text(encoding='utf-8-sig')).get('windows',{}).get('shutdown_voice_enabled') is True
    except (OSError,ValueError):return False

def feedback(config,last_id,play,now=None):
    now=int(time.time()*1000) if now is None else now
    try:
        data=json.loads(config.read_text(encoding='utf-8-sig')).get('windows',{}).get('shutdown_voice_feedback_json','{}')
        data=json.loads(data)
        event=data.get('id','')
        if not event or event==last_id:return last_id
        if data.get('accepted') is True and 0<=now-data.get('createdAt',0)<=5000 and data.get('action') in ('schedule','cancel','disable'):
            play()
        return event
    except (OSError,ValueError,TypeError):return last_id

def play_confirmation():
    import winsound
    try:
        winsound.Beep(784,90)
        winsound.Beep(1047,120)
    except RuntimeError:
        winsound.MessageBeep(winsound.MB_OK)

def main():
    a=argparse.ArgumentParser();a.add_argument('--parent',type=int,required=True);a.add_argument('--state',type=Path,required=True);a.add_argument('--model',type=Path,required=True);a.add_argument('--config',type=Path,required=True);args=a.parse_args()
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.OpenProcess.restype=ctypes.c_void_p;kernel.CreateMutexW.restype=ctypes.c_void_p
    kernel.OpenProcess.argtypes=[ctypes.c_uint32,ctypes.c_bool,ctypes.c_uint32]
    kernel.CreateMutexW.argtypes=[ctypes.c_void_p,ctypes.c_bool,ctypes.c_wchar_p]
    kernel.WaitForSingleObject.argtypes=[ctypes.c_void_p,ctypes.c_uint32];kernel.CloseHandle.argtypes=[ctypes.c_void_p]
    mutex=kernel.CreateMutexW(None,False,'Local\\VitoriaShell-Voice')
    wait=kernel.WaitForSingleObject(mutex,3000)
    if wait not in (0,128):return
    parent=kernel.OpenProcess(0x100000,False,args.parent)
    if not parent:return
    state={'status':'disabled','command':None}
    stream=None;model=None;rec=None;audio=queue.Queue(maxsize=16);last_publish=0
    # Mark existing acknowledgements consumed on launch, without replaying their sound.
    last_feedback=feedback(args.config,'',lambda:None)
    def publish():state['updatedAt']=int(time.time()*1000);write_state(args.state,state)
    def callback(data,frames,timing,status):
        try:audio.put_nowait(bytes(data))
        except queue.Full:pass
    try:
        while kernel.WaitForSingleObject(parent,0)==258:
            on=enabled(args.config)
            if not on:
                if stream is not None:stream.close();stream=None;rec=None;model=None
                while not audio.empty():
                    try:audio.get_nowait()
                    except queue.Empty:break
                state['status']='disabled';state['command']=None
            else:
                try:
                    if stream is None:
                        import sounddevice as sd
                        from vosk import Model,KaldiRecognizer,SetLogLevel
                        SetLogLevel(-1)
                        state['status']='loading';publish()
                        model=Model(str(args.model))
                        rate=int(sd.query_devices(kind='input')['default_samplerate'])
                        rec=KaldiRecognizer(model,rate,json.dumps(grammar(),ensure_ascii=False));rec.SetWords(True)
                        # Config may have changed while the model was loading.
                        if not enabled(args.config):model=None;continue
                        stream=sd.RawInputStream(samplerate=rate,blocksize=4000,dtype='int16',channels=1,callback=callback)
                        stream.start();state['status']='listening'
                    try:data=audio.get(timeout=.15)
                    except queue.Empty:data=None
                    if data is not None and rec.AcceptWaveform(data):
                        result=json.loads(rec.Result());words=result.get('result',[])
                        command=parse(result.get('text',''))
                        confidence=sum(w.get('conf',0) for w in words)/len(words) if words else 0
                        if command and confidence>=.80 and enabled(args.config):
                            state['command']={**command,'id':uuid.uuid4().hex,'createdAt':int(time.time()*1000)}
                            publish()
                except Exception:
                    if stream is not None:
                        try:stream.close()
                        except Exception:pass
                    stream=None;rec=None;model=None;state['status']='error';state['command']=None;publish();time.sleep(2)
            last_feedback=feedback(args.config,last_feedback,play_confirmation)
            if time.monotonic()-last_publish>1:publish();last_publish=time.monotonic()
            if not on:time.sleep(.25)
    finally:
        if stream is not None:stream.close()
        state['status']='stopped';state['command']=None
        try:publish()
        except OSError:pass
        kernel.CloseHandle(parent);kernel.CloseHandle(mutex)
if __name__=='__main__':main()
