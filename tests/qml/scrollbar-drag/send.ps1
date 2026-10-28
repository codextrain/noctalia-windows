Add-Type @"
using System;using System.Threading;using System.Runtime.InteropServices;
public static class BarDragInput {
[StructLayout(LayoutKind.Sequential)]struct M{public int x,y;public uint data,flags,time;public UIntPtr extra;}
[StructLayout(LayoutKind.Explicit,Size=40)]struct I{[FieldOffset(0)]public uint type;[FieldOffset(8)]public M mouse;}
[DllImport("user32.dll")]static extern uint SendInput(uint n,I[] p,int size);
[DllImport("user32.dll")]static extern int GetSystemMetrics(int n);
[StructLayout(LayoutKind.Sequential)]public struct P{public int x,y;}
[DllImport("user32.dll")]static extern IntPtr WindowFromPoint(P p);
[DllImport("user32.dll")]static extern uint GetWindowThreadProcessId(IntPtr h,out uint p);
public static uint Target(int x,int y){uint pid;GetWindowThreadProcessId(WindowFromPoint(new P{x=x,y=y}),out pid);return pid;}
static void Move(int x,int y){int l=GetSystemMetrics(76),t=GetSystemMetrics(77),w=GetSystemMetrics(78),h=GetSystemMetrics(79);SendInput(1,new[]{new I{mouse=new M{x=(x-l)*65535/(w-1),y=(y-t)*65535/(h-1),flags=0xC001}}},40);}
static void Button(uint flags){SendInput(1,new[]{new I{mouse=new M{flags=flags}}},40);}
public static void Drag(int x,int y,int end){Move(x,y);Thread.Sleep(80);Button(2);Thread.Sleep(80);try{for(int i=1;i<=20;i++){Move(x,y+(end-y)*i/20);Thread.Sleep(25);}}finally{Button(4);}}
}
"@
try {
 for($i=0;$i -lt 60;$i++){
  $c=Get-Content (Join-Path $PSScriptRoot 'config.json') -Raw|ConvertFrom-Json
  if($c.probe.ready -and $c.probe.x -and $c.probe.y){break};Start-Sleep -Milliseconds 100
 }
 if(!$c.probe.ready){throw 'No drag target'}
 $hit=[BarDragInput]::Target([int]$c.probe.x,[int]$c.probe.y)
 $proc=Get-Process -Id $hit; "target=$($proc.ProcessName) x=$($c.probe.x) y=$($c.probe.y) end=$($c.probe.end)"|Set-Content (Join-Path $PSScriptRoot "hit.txt")
 if($proc.ProcessName -ne "noctalia"){throw "Target not probe"}
 [BarDragInput]::Drag([int]$c.probe.x,[int]$c.probe.y,[int]$c.probe.end)
 Start-Sleep -Milliseconds 300
 [BarDragInput]::Drag([int]$c.probe.x,[int]$c.probe.end,[int]$c.probe.y)
 'sent'|Set-Content (Join-Path $PSScriptRoot 'input-result.txt')
}catch{$_|Out-String|Set-Content (Join-Path $PSScriptRoot 'input-result.txt')}
