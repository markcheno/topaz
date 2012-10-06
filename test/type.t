# type conversion test

def display(obj)
  println("value=",obj,", type=",get_t(obj))
end

# LHS = INT

t = 1 + 1
printf("t = 1 + 1\n")
display(t)

t = 2 + 3.5
printf("t = 2 + 3.5\n")
display(t)

t = 3 + "10"
printf("t = 3 + \"10\"\n");
display(t)

# LHS = FIXED
println

t = 1.0 + 1
printf("t = 1.0 + 1\n")
display(t)

t = 2.0 + 3.5
printf("t = 2.0 + 3.5\n")
display(t)

t = 3.0 + "3.6"
printf("t = 3.0 + \"3.6\"\n")
display(t)

# LHS = STRING
println

t = "1.0" + 1
printf("t = \"1.0\" + 1\n")
display(t)

t = "2.0" + 2.0
printf("t = \"2.0\" + 2.0\n")
display(t)


i = 1
r = 3.14159
s = "Chenoweth"
a = [69x5]

println("len of int    is: ",len(i))
println("len of real   is: ",len(r))
println("len of string is: ",len(s))
println("len of array  is: ",len(a))
