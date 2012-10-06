
class Test
 a=[5x0]
end

a = [5x0]

x = Test.new

for(n=0;n<5;n=n+1)
  a[n]=n
  x.a[n]=n+10
end

for(n=0;n<5;n=n+1)
  println("a[",n,"]=",a[n])
  println("x.a[",n,"]=",x.a[n])
end



