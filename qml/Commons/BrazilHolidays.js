// Dates are local civil dates, never UTC strings (which can shift a day in Brazil).
function key(date) {
    return date.getFullYear()+"-"+("0"+(date.getMonth()+1)).slice(-2)+"-"+("0"+date.getDate()).slice(-2);
}
function easter(year) {
    var a=year%19,b=Math.floor(year/100),c=year%100,d=Math.floor(b/4),e=b%4;
    var f=Math.floor((b+8)/25),g=Math.floor((b-f+1)/3),h=(19*a+b-d-g+15)%30;
    var i=Math.floor(c/4),k=c%4,l=(32+2*e+2*i-h-k)%7,m=Math.floor((a+11*h+22*l)/451);
    var n=h+l-7*m+114;
    return new Date(year,Math.floor(n/31)-1,n%31+1,12);
}
function yearDates(year) {
    var entries=[];
    function add(month,day,name,kind,detail) {
        var date=new Date(year,month-1,day,12);
        entries.push({key:key(date),day:date.getDate(),month:date.getMonth(),name:name,
            kind:kind||"national",detail:detail||"Feriado nacional"});
    }
    add(1,1,"Confraternização Universal");add(4,21,"Tiradentes");add(5,1,"Dia do Trabalho");
    add(9,7,"Independência do Brasil");add(10,12,"Nossa Senhora Aparecida");
    add(11,2,"Finados");add(11,15,"Proclamação da República");
    if(year>=2024)add(11,20,"Dia de Zumbi e da Consciência Negra");
    add(12,25,"Natal");
    function relative(offset,name,kind,detail) {
        var date=easter(year);date.setDate(date.getDate()+offset);
        add(date.getMonth()+1,date.getDate(),name,kind,detail);
    }
    relative(-2,"Paixão de Cristo","religious","Calendário federal; feriado religioso conforme legislação local");
    relative(-48,"Carnaval (segunda-feira)","optional","Ponto facultativo de referência; depende do calendário do órgão ou da localidade");
    relative(-47,"Carnaval (terça-feira)","optional","Ponto facultativo de referência; depende do calendário do órgão ou da localidade");
    relative(60,"Corpus Christi","optional","Ponto facultativo de referência; pode ser feriado por lei local");
    if(year===2026) {
        relative(-46,"Quarta-feira de Cinzas","optional","Ponto facultativo federal até as 14h — calendário de 2026");
        add(4,20,"Ponto facultativo federal","optional","Calendário federal de 2026");
        add(6,5,"Ponto facultativo federal","optional","Calendário federal de 2026");
        add(10,28,"Dia do Servidor Público federal","optional","Ponto facultativo federal — calendário de 2026");
        add(12,24,"Véspera de Natal","optional","Ponto facultativo federal após as 13h — calendário de 2026");
        add(12,31,"Véspera de Ano Novo","optional","Ponto facultativo federal após as 13h — calendário de 2026");
    }
    return entries.sort(function(a,b){return a.key.localeCompare(b.key);});
}
function monthDates(year,month,includeOptional) {
    return yearDates(year).filter(function(e){return e.month===month&&(includeOptional||e.kind!=="optional");});
}
