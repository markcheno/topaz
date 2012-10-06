# calculate pi to 1000 digits

a=10000;c=2800;e=0;f=[2800x2000,0]

while(c)
	g=c*2;d=0;b=c
  	while(b)
		d=d*b
		d=d+f[b]*a
		g=g-1
		f[b]=d%g
		d=d/g
		g=g-1
		b=b-1
	end
	c=c-14
	printf("%.4d",e+d/a)
	e=d%a
end
println
