class A

 LEN=10

 a=[10x0]

 def set(index,value)
  a[index] = value
 end
 
 def get(index)
  return a[index]
 end
 
 def prt(index)
  println("a[",index,"]=",a[index])
 end
 
end

x = A.new

println("length=",x.LEN)

x.set(5,"Mark")
x.set(6,"Cheno")

x.prt(5)
x.prt(6)
