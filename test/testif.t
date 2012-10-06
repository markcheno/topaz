# testif.m1

def inRange
  println("In Range")
end

def outRange
  println("Out of Range")
end

def xxx
  println("xxx")
end

SIZE=5
x=80

if (x < 0) or (x > 160-SIZE) then
  outRange
else
  inRange
end

if x > 160-SIZE then
  outRange
else
  inRange
end

