# sieve of eratosthenes

a=[0,0,998x1];

for( i=2; i<len(a); i=i+1 ) 
  if a[i] then
    for( j=i; j<len(a)/i; j=j+1 ) 
	  a[i*j]=0
    end
  end
end

for( i=2; i<len(a); i=i+1 )
  if a[i] 
    printf('%4d\n',i)
  end
end
println
