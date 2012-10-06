#/usr/bin/perl -w 

@list = glob("*.t");

foreach $program (@list)
{
	$name = substr($program,0,length($program)-1);
	
	$listing       = $name . "lis";
	$output        = $name . "out";
	$bytecode      = $name . "bin";

	$new_listing   = $name . "lis.tmp";
	$new_output    = $name . "out.tmp";
	$new_bytecode  = $name . "bin.tmp";
	system("../tc -l -o $new_bytecode $program >$new_listing");
	system("../tr $new_bytecode >$new_output");
	system("diff $listing  $new_listing");
	system("diff $output   $new_output");
	system("diff $bytecode $new_bytecode");
	system("rm *.tmp");
}
