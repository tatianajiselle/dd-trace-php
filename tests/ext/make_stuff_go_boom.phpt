--TEST--
TODO
--FILE--
<?php
$NUM = 100000;
const BLAH = 'teSt';

for($i = 0; $i < 20 ; $i++) {
    dd_trace_reset();

    for($test_no = 0; $test_no < $NUM; $test_no++){
        dd_trace(BLAH, function($arg) use ($test_no){
            return "HOOK " . $test_no . ' ' . test($arg) . PHP_EOL;
        });
    }
}

function test($arg) {
    return "FN " . $arg;
};

echo test($NUM);

?>
--EXPECT--
