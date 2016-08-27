<?php
   	include("connect.php");
   	
   	$link=Connection();

	$tempciel=$_POST["temperatureciel"];

	$tempambient=$_POST["tempambient"];
	$pluie=$_POST["detectionpluie"];
	$tempsol=$_POST["tempsol"];
	$skysqm=$_POST["sqmaverage"];


	$query = "INSERT INTO Temperatures (NomSonde, Valeur) 
		VALUES ('TemperatureCiel','$tempciel')"; 
	$query1 = "INSERT INTO Humidite (NomSonde, Valeur) 
		VALUES ('detecpluie','$pluie')"; 
	$query2 = "INSERT INTO Temperatures (NomSonde, Valeur) 
		VALUES ('TemperatureAmbient','$tempambient')"; 
	$query3 = "INSERT INTO Temperatures (NomSonde, Valeur) 
		VALUES ('TemperatureSol','$tempsol')"; 
	$query4 = "INSERT INTO SQMsky (NomSonde, Valeur) 
		VALUES ('SkyQuality','$skysqm')"; 
file_put_contents(log.txt, $tempciel);
   	
   	mysql_query($query,$link);
   	mysql_query($query1,$link);
   	mysql_query($query2,$link);
   	mysql_query($query3,$link);
   	mysql_query($query4,$link);

   	mysql_close($link);


?>
