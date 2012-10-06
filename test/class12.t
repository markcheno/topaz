class Constant
 protected
  PI=3.14159
end

class Math
  def squared(x)
    return x*x
  end
end

class Test < Constant
  
  pi = PI
 private
  array = [3,4,5]
 public
  def p
   a = [1,2,3]
   println("a[0]=",a[0])
   println("array[0]=",array[0])
   v = Math.new
   println("pi=",pi)
   println("pi**2=",v.squared(PI))
  end
  
end


def func1
 z = [3,4,5]
 println("z[0]=",z[0])
end

x = Test.new
x.p

println("x.pi=",x.pi)

func1
