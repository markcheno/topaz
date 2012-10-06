# array initializer test

a1 = [1,2,3,4,5]

for(i=0;i<5;i=i+1)
  println("array1[",i,"]=",a1[i])
end


a2 = [1.1,2.2,3.3,4.4,5.5]

for(i=0;i<5;i=i+1)
  println("array2[",i,"]=",a2[i])
end


a3 = ["Mark","Debby","Taylor","Colin","Spot"]

for(i=0;i<5;i=i+1)
  println("array3[",i,"]=",a3[i])
end

a4 = [1,2.0,"Three",4,"five"]

for(i=0;i<5;i=i+1)
  println("array4[",i,"]=",a4[i])
end

a5 = ["start",1,2,3,5 x 69,10x2,3,"three",2x 3.1415,"end",0]

for(i=0;a5[i];i=i+1)
  println("array5[",i,"]=",a5[i])
end

