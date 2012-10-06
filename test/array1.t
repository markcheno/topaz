# array test

a = [5x0]

for( x=0; x<5; x=x+1 )
 a[x] = x+1
 println('a[',x,']=',a[x])
end

println

a[3] = 1

a[2] = a[4] + 2

for( x=0; x<5; x=x+1 )
 println('a[',x,']=',a[x])
end
