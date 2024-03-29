<?// Copyright (c) 2018 Artur Wiebe <artur@4wiebe.de>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


error_reporting(E_ALL & ~E_NOTICE & ~E_STRICT & ~E_DEPRECATED);


function conf($file=null, $section=null, $key=null, $default=null) {
	static $conf = null;
	
	if (is_null($conf)) {
		$conf = [];
		foreach (glob('/etc/app/*.conf') as $path) {
			$conf[basename($path, '.conf')] = parse_ini_file($path, true);
		}
	}
	
	if (is_null($file))
		return $conf;
	
	if (is_null($section))
		return array_key_exists($file, $conf);
	
	if (is_null($key))
		return array_key_exists($file, $conf) && array_key_exists($section, $conf[$file]); 
	
	return (array_key_exists($file, $conf) && array_key_exists($section, $conf[$file]) && array_key_exists($key, $conf[$file][$section]) && strlen($conf[$file][$section][$key])) ? $conf[$file][$section][$key] : $default;
}


function setup($section=null, $key=null) {
	static $setup;
	if (!isset($setup)) {
		$setup = json_decode(exec('python -c "from shared.setup import setup; import sys,json; json.dump(setup,sys.stdout)"'), true) ?: [];
	}
	return is_null($section) ? $setup : (is_null($key) ? $setup[$section] : ($setup[$section][$key] ?? null));
}


function awaitCondition($condition, $timeout, $sleep=false, $settle=0, $result=false, $ifnot=false) {
	ll("'### awaitCondition({$condition})");
	if ($sleep) {
		ll("sleep {$sleep}");
	}
	ll('com_store()');
	ll('com_putLong("/timeout", system.clock + '.$timeout.')');
	ll('com_putLong("/settle",  system.clock + '.$settle.')');
	ll('com_putBool("/result",  false)');
	ll('do');
		$poll = min(20, $timeout);
		lll("if {$condition} then");
			llll('if system.clock >= com_getLong("/settle") then');
				lllll('com_putBool("/result", true)');
			llll('else');
				lllll("sleep {$poll}");
			llll('end if');
		lll('else');
			llll("sleep {$poll}");
			llll('com_putLong("/settle", system.clock + '.$settle.')');
		lll('end if');
	ll('loop until com_getBool("/result") or system.clock > com_getLong("/timeout")');
	if ($result) {
		ll($result.' = com_getBool("/result")');
	}
	if ($ifnot) {
		ll('if not com_getBool("/result") then');
			lll('com_restore()');
			call($ifnot);
		ll('else');
			lll('com_restore()');
		ll('end if');
	} else {
		ll('com_restore()');
	}
	ll("'### end awaitCondition");
}


function debug() {
	return file_exists('/etc/app/debug');
}


function l($basic='')	{ echo $basic."\n"; }
function ll($basic)		{ echo "\t".$basic."\n"; }
function lll($basic)	{ echo "\t\t".$basic."\n"; }
function llll($basic)	{ echo "\t\t\t".$basic."\n"; }
function lllll($basic)	{ echo "\t\t\t\t".$basic."\n"; }

function b($bool) {
	return $bool?'true':'false';
}
function s($str) {
	if (!is_string($str))
		$str = json_encode($str);
	return '"'.wordwrap(strtr($str, '`"', "'`"), 80, '"+"', true).'"';
}
function ifs($if, $then, $else='') { return $if ? $then : $else; }

function call($parts, $reverse=false) {
	if (is_array($parts)) {
		foreach ($reverse?array_reverse($parts):$parts as $k=>$v)
			if (is_int($k))
				call($v);
	} elseif (is_callable($parts)) {
		$parts();
	} elseif (is_string($parts)) {
		lll($parts);
	}
}

function hashsum($whatever) {
	if (is_callable($whatever)) {
		ob_start();
		$whatever();
		return hashsum(ob_get_clean());
	}
	return md5(serialize($whatever));
}


function dump($var, $caption='dump') {
	file_put_contents('php://stderr', $caption.":\n".print_r($var, true)."\n\n");
}
