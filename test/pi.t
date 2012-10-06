# A spigot algorithm for the Digits of \pi, Stanley Rabinowitz
# and Stan Wagon, Amer.Math.Monthly, March 1995, 195-203

n=100
l=10*n/3
a=[333x2]

nines=0
predigit=0

for( j=0; j<n; j=j+1 )

	q=0
	for( i=l; i; i=i-1 )
		x = 10*a[i-1] + q * i
		a[i-1] = x % (2*i-1)
		q = x / (2*i-1)
    end
	
	a[0] = q % 10
	q = q / 10
	if q==9 
		nines=nines+1
	elsif q==10
		printf("%d",predigit+1)
		predigit=0
        for( ; nines; nines=nines-1 )
		  printf("%d",0)
		end
	else
		printf("%d",predigit)
		predigit=q
        for( ; nines; nines=nines-1 )
		  printf("%d",9)
		end
	end
end
printf("%d\n",predigit)

