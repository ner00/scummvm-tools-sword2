<style>
.wrap { 
	white-space: pre-wrap;      /* CSS3 */   
	white-space: -moz-pre-wrap; /* Firefox */    
	white-space: -pre-wrap;     /* Opera 6- */   
	white-space: -o-pre-wrap;   /* Opera 7+ */    
	word-wrap: break-word;      /* IE */
	font-family: "Lucida Console", "Courier New", monospace;
}

.mono-font { 
	font-family: "Lucida Console", "Courier New", monospace;
}
</style>
<?php
// FILE LOOKUP TABLE
//$start_offset_flt = "D2400000";
$start_offset_flt = swap_byteorder("0001AE45");

// TEXT RESOURCE INDEX TABLE
$resource_offset = hexdec(swap_byteorder($start_offset_flt));//0x1ae45;
$resource_size_old = 0x790;
$resource_size_new = 0x848;
$shift_bytes = $resource_size_new - $resource_size_old;
if ($shift_bytes < 0) {$shift_op = "-"; $shift_bytes = $resource_size_old - $resource_size_new;} else $shift_op = "+";
$shift_bytes = strtoupper(dechex($shift_bytes));
//$shift_bytes = "B8";

/*
echo "old_div/2: " . ($resource_size_old % 2) . "\n";
echo $resource_size_old / 2 . "\n\n";
echo "new_div/2: " . ($resource_size_new % 2) . "\n";
echo $resource_size_new / 2 . "\n\n";
echo "old_div/4: " . ($resource_size_old % 4) . "\n";
echo $resource_size_old / 4 . "\n\n";
echo "new_div/4: " . ($resource_size_new % 4) . "\n";
echo $resource_size_new / 4 . "\n\n";
$sign = "+";
$diff = $resource_size_new - $resource_size_old;
if ($diff < 0) {$sign = "-"; $diff = $resource_size_old - $resource_size_new;}
echo $sign . dechex($diff);
exit;
*/

// BYTE SHIFTING RESOURCE INDEX TABLE
//$start_offset_res = "EB010000"; 	// +F
//$start_offset_res = "B2020000"; 	// -F
//$start_offset_res = "E0020000"; 	// +25
/*
//hexdec(swap_byteorder($offset)) > hexdec(swap_byteorder($start_offset_flt))
echo hexdec(swap_byteorder("B9530400")); //E1 53 04 00
echo "<br>"; //C1 53 04 00
echo hexdec(swap_byteorder("45AE0100"));
exit;
*/

header("Content-Type: text/html; charset=ISO-8859-1");


function read_txt_res_table($filename, $offset, $size) {
	$file = fopen($filename, "rb");

	// Seek to the starting offset
	fseek($file, $offset);

	// Initialize an array to store the data
	$data = array();

	// Set the length to read
	$length = $size;

	// Loop until we have read the desired number of bytes
	while ($length > 0) {
		// Read 4 bytes from the file and store them in the array
		$bytes = fread($file, 4);
		$data[] = bin2hex($bytes);
		
		// Decrement the length by the number of bytes read
		$length -= strlen($bytes);
	}

	fclose($file);

	// Convert lowercase to uppercase
	$data = array_map("strtoupper", $data);

	//print_r($data);
	return $data;
}


function read_file_lookup_table($filename, $offset) {
	$file = fopen($filename, "rb");

	// Seek to the starting offset
	fseek($file, $offset);

	// Initialize an array to store the data
	$data = array();

	// Loop until we have reched the end of the file
	while (!feof($file)) {
		// Read 4 bytes from the file and convert them to hexadecimal
		$bytes1 = bin2hex(fread($file, 4));
		
		// Read the next 4 bytes from the file and convert them to hexadecimal
		$bytes2 = bin2hex(fread($file, 4));
		
		// Check if we have reached the end of the file
		if(feof($file)) break;
		
		// Store the hexadecimal strings in the array
		$data[] = array($bytes1, $bytes2);
	}

	fclose($file);

	function to_uppercase($inner_array) {
		return array_map("strtoupper", $inner_array);
	}

	// Convert the values in the data array to uppercase
	$data = array_map("to_uppercase", $data);

	return $data;
}


function read_txt_resource($filename, $offset) {
	$file = fopen($filename, "rb");

	// Seek to the starting offset
	fseek($file, $offset);

	$dialogue = "";

	$dialogueID = bin2hex(fread($file, 2));

	// Read the file one byte at a time until we encounter a null byte
	while(($char = fgetc($file)) !== "\0") {
		$dialogue .= $char;
	}

	fclose($file);

	//return array with ID and text;
	return array($dialogueID, $dialogue);
}


function read_simple($filename, $offset, $size) {
	$file = fopen($filename, "rb");

	// Seek to the starting offset
	fseek($file, $offset);

	$data = "";

	$data = bin2hex(fread($file, $size));

	fclose($file);

	//return array with ID and text;
	return $data;
}


function swap_byteorder($hex) {
	if (strlen($hex) % 2 == 1) $hex = "0" . $hex;
	$characters = str_split($hex, 2);
	$reversed = array_reverse($characters);
	$result = implode("", $reversed);
	return $result;
}


echo "<h3>Lookup Table Offset<br>&nbsp;&nbsp;&nbsp; * Shifted bytes: $shift_op$shift_bytes</h3>";
echo "<div class=\"mono-font\" onclick='window.getSelection().selectAllChildren(this)'>";
$lookup_table_offset = read_simple("TEXT.CLU", 0, 4);
if ($shift_op == "-") $dec = hexdec(swap_byteorder($lookup_table_offset)) - hexdec($shift_bytes);
else $dec = hexdec(swap_byteorder($lookup_table_offset)) + hexdec($shift_bytes);
$shifted_lookup_table_offset = sprintf("%08X", $dec);
$shifted_lookup_table_offset = swap_byteorder($shifted_lookup_table_offset);
echo $shifted_lookup_table_offset;
echo "</div><br><br>";

$data = read_file_lookup_table("TEXT.CLU", hexdec(swap_byteorder($lookup_table_offset)));
$lookup_table_count = (count($data, COUNT_RECURSIVE) - count($data)) / 2;
echo "<h3>File Lookup Table ($lookup_table_count index offsets + $lookup_table_count chunk sizes)<br>&nbsp;&nbsp;&nbsp; * Starting index offset: $start_offset_flt | Shifted bytes: $shift_op$shift_bytes</h3>";
echo "<div class=\"wrap\" onclick='window.getSelection().selectAllChildren(this)'>";
$keys = array_keys($data);
$shift = -1;
$id = -1;
for($i = 0; $i < count($data); $i++) {
    foreach($data[$keys[$i]] as $key => $offset) {
    	if ($offset == $start_offset_flt) $shift = 1;
    	if (
			$key == 1 && $shift == 1 ||
			$key == 0 && hexdec(swap_byteorder($offset)) > hexdec(swap_byteorder($start_offset_flt))
		) {
    		//if ($key == 0) echo "start shifting<br>";
			$shift = 0;
			if ($shift_op == "-") $dec = hexdec(swap_byteorder($offset)) - hexdec($shift_bytes);
			else $dec = hexdec(swap_byteorder($offset)) + hexdec($shift_bytes);
			$shifted_hex = sprintf("%08X", $dec);
			$shifted_hex = swap_byteorder($shifted_hex);
    		//echo "$key : $offset<br>";
			//echo "$key : $shifted_hex<br>";
    		echo "<font color=\"red\">$shifted_hex</font>";
    	} else {
        	//echo "$key : $offset<br>";
        	echo $offset;
    	}
    }
    //echo "<br>";
}
echo "</div><br><br>";

/*
$data = read_txt_res_table("TEXT.CLU", $resource_offset, $resource_size_old);
$resource_items = hexdec(swap_byteorder($data[11]));
echo "<h3>Text Resource Table ($resource_items offsets)<br>&nbsp;&nbsp;&nbsp; * Starting offset: " . strtoupper(dechex($resource_offset)) . " | Shifted bytes: $shift_op$shift_bytes</h3>";
echo "<div class=\"wrap\" onclick='window.getSelection().selectAllChildren(this)'>";
$shift = -1;
foreach ($data as $offset) {
	if ($shift_bytes != "" && strtolower($offset) == strtolower($start_offset_res)) {
		$shift = 1;
		//echo "start shifting<br>";
	}
	if ($shift == 1) {
		if ($shift_op == "-") $dec = hexdec(swap_byteorder($offset)) - hexdec($shift_bytes);
		else $dec = hexdec(swap_byteorder($offset)) + hexdec($shift_bytes);
		$shifted_hex = sprintf("%08X", $dec);
		$shifted_hex = swap_byteorder($shifted_hex);
		//echo "$offset<br>";
		//echo "$shifted_hex<br>";
		echo "<font color=\"red\">$shifted_hex</font>";
	} else {
		//echo "$offset<br>";
		echo $offset;
	}
	//echo "<br>";
}
echo "</div><br><br>";
*/


$txt_res_table = read_txt_res_table("TEXT.CLU", $resource_offset, $resource_size_old);
echo "<h3>Text Resource Dialogue<br>&nbsp;&nbsp;&nbsp; * Starting offset: " . strtoupper(dechex($resource_offset)) . "</h3>";
echo "<div class=\"mono-font\">";
$i = 0;
foreach ($txt_res_table as $offset) {
	$i++;
	if ($i >= 3 && $i <= 7) {
		if (!isset($resource_id)) $resource_id = "";
		$resource_id .= hex2bin($offset);
		if ($i == 7) echo "<h4>$resource_id</h4>";
	}
	if ($i == 12) {
		$dialogue_items = hexdec(swap_byteorder($offset));
	}
	if ($i > 12 && $i - 12 <= $dialogue_items) {
		$base_offset = dechex($resource_offset);
		$dec = hexdec($base_offset) + hexdec(swap_byteorder($offset));
		$txt_offset = sprintf("%08X", $dec);
		list($dialogueID, $dialogue) = read_txt_resource("TEXT.CLU", hexdec($txt_offset));
		$dialogueID = hexdec(swap_byteorder($dialogueID));
		$dialogue_length = strtoupper(dechex(strlen($dialogue)));
		$idx_str = str_replace("~", "&nbsp;", str_pad("IDX", strlen($dialogueID), "~"));
		echo "$dialogueID: $dialogue<br>";
		echo "$idx_str: $offset | FILE: $txt_offset | Length: $dialogue_length<br><br>";
	}
}
echo "</div>";
?>