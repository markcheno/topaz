#/usr/bin/perl -w 

system("rm -f *.out *.bin *.lis");
@list = glob("*.t");

foreach $program (@list)
{
	$name = substr($program,0,length($program)-1);
	$listing  = $name . "lis";
	$output   = $name . "out";
	$bytecode = $name . "bin";
	system("../tc -l -o $bytecode $program >$listing");
	system("../tr $bytecode >$output");
}
