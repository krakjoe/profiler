<?php
profiler_enable();
class my_test_class {
	public function my_test_method(){
		return my_function_call();
	}
}
/**
* my_function_call
* @return void
**/ 
function my_function_call(){
	my_other_call();
}
function my_other_call() {
	return file_get_contents("http://www.google.com");
}

print_r(get_loaded_extensions());
my_function_call();

$test = new my_test_class();
$test->my_test_method();


profiler_output("/tmp/profile.callgrind");
?>
