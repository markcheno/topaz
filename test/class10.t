
class Parent				# parent class

  no_modifier=10			# available to everyone
  
  private priv=20			# internal to this class

  protected prot=30 		# available to subclasses as well
  
  public pub=40 			# available to everyone

  # report on contents of this class
  public def parentReport
    println("no_modifier=",no_modifier)
	println("priv=",priv)
	println("prot=",prot)
	println("pub =",pub)  
  end
end
  
class Child < Parent  # child class
  
  def childReport
    println("no_modifier=",no_modifier)
	println("Cannot access priv variable")
	# should cause error -
	#println("priv=",priv)
	println("prot=",prot)
	println("pub=",pub)
  end
  
end

dad = Parent.new
son = Child.new

dad.parentReport
son.childReport

# should be ok
println("dad.no_modifier=",dad.no_modifier)
println("dad.pub=",dad.pub)
println("son.no_modifier=",son.no_modifier)
println("son.pub=",son.pub)

# should cause errors
# println("son.prot=",son.prot)
# println("dad.prot=",dad.prot)

# should cause errors
# println("son.priv=",son.priv)
# println("dad.priv=",dad.priv)

