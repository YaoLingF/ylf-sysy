global @n_0=alloc i32,zeroinit
fun @QuickSort(%arr:*i32,%low:i32,%high:i32):i32{
%entry1:
@arr_1=alloc *i32
store %arr,@arr_1
@low_1=alloc i32
store %low,@low_1
@high_1=alloc i32
store %high,@high_1
%0=load @low_1
%1=load @high_1
%2=lt %0,%1
br %2,%then_1,%end_1
%then_1:
@i_2=alloc i32
%3=load @low_1
store %3,@i_2
@j_2=alloc i32
%4=load @high_1
store %4,@j_2
@k_2=alloc i32
%5=load @low_1
%6=load @arr_1
@L7=getptr %6,%5
%8=load @L7
store %8,@k_2
jump %whilecond_1
%whilecond_1:
%9=load @i_2
%10=load @j_2
%11=lt %9,%10
br %11,%whilebody_1,%whileend_1
%whilebody_1:
jump %whilecond_2
%whilecond_2:
%12=load @i_2
%13=load @j_2
%14=lt %12,%13
@landans_1=alloc i32
br %14,%else_2,%then_2
%then_2:
store 0,@landans_1
jump %end_2
%else_2:
%15=load @j_2
%16=load @arr_1
@L17=getptr %16,%15
%18=load @k_2
%19=sub %18,1
%20=load @L17
%21=gt %20,%19
%22=ne 0,%21
store %22,@landans_1
jump %end_2
%end_2:
%23=load @landans_1
br %23,%whilebody_2,%whileend_2
%whilebody_2:
%24=load @j_2
%25=sub %24,1
store %25,@j_2
jump %whilecond_2
%whileend_2:
%26=load @i_2
%27=load @j_2
%28=lt %26,%27
br %28,%then_3,%end_3
%then_3:
%29=load @i_2
%30=load @arr_1
@L31=getptr %30,%29
%32=load @j_2
%33=load @arr_1
@L34=getptr %33,%32
%35=load @L34
store %35,@L31
%36=load @i_2
%37=add %36,1
store %37,@i_2
jump %end_3
%end_3:
jump %whilecond_3
%whilecond_3:
%38=load @i_2
%39=load @j_2
%40=lt %38,%39
@landans_2=alloc i32
br %40,%else_4,%then_4
%then_4:
store 0,@landans_2
jump %end_4
%else_4:
%41=load @i_2
%42=load @arr_1
@L43=getptr %42,%41
%44=load @L43
%45=load @k_2
%46=lt %44,%45
%47=ne 0,%46
store %47,@landans_2
jump %end_4
%end_4:
%48=load @landans_2
br %48,%whilebody_3,%whileend_3
%whilebody_3:
%49=load @i_2
%50=add %49,1
store %50,@i_2
jump %whilecond_3
%whileend_3:
%51=load @i_2
%52=load @j_2
%53=lt %51,%52
br %53,%then_5,%end_5
%then_5:
%54=load @j_2
%55=load @arr_1
@L56=getptr %55,%54
%57=load @i_2
%58=load @arr_1
@L59=getptr %58,%57
%60=load @L59
store %60,@L56
%61=load @j_2
%62=sub %61,1
store %62,@j_2
jump %end_5
%end_5:
jump %whilecond_1
%whileend_1:
%63=load @i_2
%64=load @arr_1
@L65=getptr %64,%63
%66=load @k_2
store %66,@L65
@tmp_2=alloc i32
jump %end_1
%end_1:
ret 0
}

fun @main():i32{
%entry2:
store 10,@n_0
@a_1=alloc [i32,10]
@list0=getelemptr @a_1,0
store 0,@list0
@list1=getelemptr @a_1,1
store 0,@list1
@list2=getelemptr @a_1,2
store 0,@list2
@list3=getelemptr @a_1,3
store 0,@list3
@list4=getelemptr @a_1,4
store 0,@list4
@list5=getelemptr @a_1,5
store 0,@list5
@list6=getelemptr @a_1,6
store 0,@list6
@list7=getelemptr @a_1,7
store 0,@list7
@list8=getelemptr @a_1,8
store 0,@list8
@list9=getelemptr @a_1,9
store 0,@list9
@L79=getelemptr @a_1,0
store 4,@L79
@L80=getelemptr @a_1,1
store 3,@L80
@L81=getelemptr @a_1,2
store 9,@L81
@L82=getelemptr @a_1,3
store 2,@L82
@L83=getelemptr @a_1,4
store 0,@L83
@L84=getelemptr @a_1,5
store 1,@L84
@L85=getelemptr @a_1,6
store 6,@L85
@L86=getelemptr @a_1,7
store 5,@L86
@L87=getelemptr @a_1,8
store 7,@L87
@L88=getelemptr @a_1,9
store 8,@L88
@i_1=alloc i32
store 0,@i_1
@tmp_1=alloc i32
store 9,@tmp_1
%89=getelemptr @a_1,0
%90=load @i_1
%91=load @tmp_1
%92=call @QuickSort(%89,%90,%91)
store %92,@i_1
jump %whilecond_4
%whilecond_4:
%93=load @i_1
%94=load @n_0
%95=lt %93,%94
br %95,%whilebody_4,%whileend_4
%whilebody_4:
@tmp_2=alloc i32
%96=load @i_1
@L97=getelemptr @a_1,%96
%98=load @L97
store %98,@tmp_2
%99=load @tmp_2
call @putint(%99)
store 10,@tmp_2
%100=load @tmp_2
call @putch(%100)
%101=load @i_1
%102=add %101,1
store %102,@i_1
jump %whilecond_4
%whileend_4:
ret 0
}