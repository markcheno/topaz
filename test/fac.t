# Calculate Factorial of n
def fact(n)

  if n == 0 then return 1 end
  
  f=1
  while n > 0 do
    f = f*n
    n = n-1
  end
  return f
  
end

x = 7

println("factorial of ",x," = ",fact(x))
