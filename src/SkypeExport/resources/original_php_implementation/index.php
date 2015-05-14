<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html>
	<head>
		<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
		<meta http-equiv="X-UA-Compatible" content="IE=edge" /> <!-- tells IE to use standards-compliant mode and the newest engine -->
		<title>Skype Chat Analyzer</title>
		<style type="text/css">
			<?php echo file_get_contents("style_compact_data.css").PHP_EOL; ?>
		</style>
	</head>
	<body>
<?php
	// Instantiate Skype Parser
	require_once("skypeparser.inc.php");
	$sp = new SkypeParser("main.db");
	
	// Output the list of Skypers and create the links to view history between you and that person
	/*$aSkypers = $sp->getSkypers();
	foreach ($aSkypers as $sUserID => $aDisplayNames) {
		echo "<span>UserID: ${sUserID} (DisplayNames: ".join(", ", $aDisplayNames).")</span><br />".PHP_EOL;
	}*/
	
	/* FIXME: View history for the selected person. Also offer an option to download the history as html or xml or pdf or whatever. */
	$sp->showHistory("grince.farbgold", false); // FIXME: hardcoded, but take this from the user later
	//$sp->showHistory("coolguy123", false); // multiple logs can be parsed one after the other, and will output cleanly
	//$sp->showHistory(74595, true);
?>
	</body>
</html>
