
class Cart

 protected
  items=0
 
 public
  def addItem
    items=items+1
  end
 
  def getItems
    return items
  end
  
end

class NamedCart < Cart

 private
  owner="nobody"
  
 public
  def init(owner)
    self.owner=owner
  end
  
  def setOwner(owner)
    self.owner=owner
  end
  
  def getOwner
    return owner
  end

  def removeItem
    super.items=super.items-1
  end
  
end

c = NamedCart.new("Mark")

c.addItem		# 1
c.addItem		# 2
c.addItem		# 3
c.removeItem	# 2

printf("cart owner = %s\n",c.getOwner)
printf("number of items = %d\n",c.getItems)

# should be error
#println("c.items=",c.items)

