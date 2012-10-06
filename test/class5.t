
PI=3.1415

def printPI
 println("PI=",PI)
end

LEN=10
for(x=1;x<LEN;x=x+1)
  println("x=",x)
end

class Test
 f1=1
 f2=2
 f3=3
 def p
   printPI
 end
end

o = Test.new

o.f1 = PI

println("o.f1=",o.f1)
println("o.f2=",o.f2)
println("o.f3=",o.f3)

printPI

o.p
