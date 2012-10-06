
class Point

  x=2
  y=2
  
  def printXY
    println("x=",x)
	println("y=",y)
  end
  
end


class Point3d < Point

  z=2
  
  def printXYZ
    println("x=",x)
	println("y=",y)
	println("z=",z)
  end
  
end

p2 = Point3d.new

p2.x = 4
p2.y = 5
p2.z = 6

p2.printXY

p2.printXYZ

println("p2.x=",p2.x)
println("p2.y=",p2.y)
println("p2.z=",p2.z)
