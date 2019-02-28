<?php

const NAME = "ehlo";

dd_trace(NAME, function(){
    return 'HOOK ' . ehlo();
});

function ehlo() {
    return "Ehlo". PHP_EOL;
}

echo ehlo();


dd_trace(NAME, function(){
    return 'HOOK1 ' . ehlo();
});

echo ehlo();

dd_trace(NAME, function(){
    return 'HOOK2 ' . ehlo();
});

echo ehlo();

dd_trace(NAME, function(){
    return 'HOOK3 ' . ehlo();
});

echo ehlo();

dd_trace(NAME, function(){
    return 'HOOK4 ' . ehlo();
});

echo ehlo();

dd_untrace(NAME);

echo ehlo();
