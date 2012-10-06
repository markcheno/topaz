
class Grandfather
 f1 = "Grandfather"
 def init
   println("Grandfather: init")
 end
 def xx
   println("Grandfather: xx")
 end
end


class Father < Grandfather
  f2 = "Father"
  def init
    super.init
    println("Father: init")
  end
end

class Son < Father
  f3 = "Son"
  def init
    super.init
    println("Son: init")
    xx
  end
end

x = Son.new
x.xx

println("x.f1=",x.f1)
println("x.f2=",x.f2)
println("x.f3=",x.f3)
