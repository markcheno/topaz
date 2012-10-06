# alternate constructor test

class Point

  x=10
  y=10
  z=10
  
 def init(x,y)
   self.x=x
   self.y=y
 end

 def print
   println("x=",x)
   println("y=",y)
   println("z=",z)
 end
 
end


p1 = Point.new
p1.print

p2 = Point.new(69,69)
p2.print

print("p1.x=",p1.x,"\n")
print("p1.y=",p1.y,"\n")
print("p1.z=",p1.z,"\n")

print("p2.x=",p2.x,"\n")
print("p2.y=",p2.y,"\n")
print("p2.z=",p2.z,"\n")
