<?php

	function Connection(){
		$server="localhost";
		$user="observatory";
		$pass="zQnVBkc1fDM7";
		$db="observatory";
	   	
		$connection = mysql_connect($server, $user, $pass);

		if (!$connection) {
//		   	$file = 'connect.txt';
//file_put_contents($file, "connectiondead", FILE_APPEND | LOCK_EX); 

	    	die('MySQL ERROR: ' . mysql_error());
		}
//		   	$file = 'connect.txt';
//file_put_contents($file, "connectionok", FILE_APPEND | LOCK_EX); 

		mysql_select_db($db) or die( 'MySQL ERROR: '. mysql_error() );

		return $connection;
	}
?>
