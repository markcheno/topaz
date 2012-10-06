
class Grandfather
  f1 = "Grandfather"
  age = 75
end

class Father < Grandfather
  f2 = "Father"
  age = 50
end

class Son < Father
  f3 = "Son"
  age = 25
end

x = Son.new
println("age=",x.age)
println("f1=",x.f1)
println("f2=",x.f2)
println("f3=",x.f3)

y = Father.new
println("age=",y.age)
println("f1=",y.f1)
println("f2=",y.f2)


