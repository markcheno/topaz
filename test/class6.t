
class Test

  private
  
	x=1; y=2

  public
  
    def getX
		return self.x
	end
	
	def setX(x)
		self.x=x
	end

	def printX
		println("x=",self.x)
	end
	
	def getY
		return self.y
	end
	
	def setY(y)
		self.y=y
	end
	
	def printY
		println("y=",self.y)
	end

end

z = Test.new

z.printX
z.printY

z.setX(100)
z.setY(200)

println("z.x=",z.getX)
println("z.y=",z.getY)


# error...
#z.x = 1000
#z.printX
#
#z.y = 2000
#z.printY
