# Calculate Fibonacci of 20 (6765)

def fib(n)
	
	if n < 2 then
	  return n
	else
	  return fib(n-2)+fib(n-1)
	end

end

for(x=1;x<=20;x=x+1)
 printf("fib(%d)=%d\n",x,fib(x))
end

