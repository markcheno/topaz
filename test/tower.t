# Towers of Hanoi

def move(src,dest)
  println('Move ',src,' to ',dest)
end

def transfer(n,src,dest,other)
  if n > 0 
  	transfer(n-1,src,other,dest)
	move(src,dest)
	transfer(n-1,other,dest,src)
  end
end

num=5

println('Towers of Hanoi for: ',num,' disks')
transfer(num,1,3,2)
