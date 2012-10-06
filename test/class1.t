class T1

 f1 = 1;
 f2 = 2;
 f3 = 3;

 def p(val1,val2,val3,val4)
   println("val1=",val1)
   println("val2=",val2)
   println("val3=",val3)
   println("val4=",val4)
   f1 = val1
   f2 = val2
   f3 = val3
 end

 def pf
   println("f1=",f1)
   println("f2=",f2)
   println("f3=",f3)
 end;
 
 def get_f1
   return f1
 end
 
 def get_f2
   return f2
 end
 
 def get_f3
   return f3
 end
 
end;

def xxx
 println("xxx")
end;

x = T1.new

x.pf

x.p("Mark","Deb","Tay","Col")

x.pf

x.f1 = 11;
x.f2 = 22;
x.f3 = 33;

x.pf

f1 = x.get_f1
f2 = x.get_f2
f3 = x.get_f3

println("f1=",f1)
println("f2=",f2)
println("f3=",f3)

xxx
