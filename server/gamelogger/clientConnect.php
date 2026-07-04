<?php

	error_reporting(E_ALL);
	ini_set("display_errors", 1);	
	

$request = $_REQUEST["request"];
include("database.php");

switch ($request) {
case "settings":
	// For settings, simply output the settings file and precede it with the server time.
	echo "#\tserverTime\t".time()."\n";
	readfile("settings.txt");
	break;
case "updateSession":

	// Store APM
	$player = $_REQUEST["player"];
	$apm = $_REQUEST["apm"];
	$minute = time();
	$minute = $minute - $minute%60;
	appendAPMminute($mysqli, $player, $apm, $minute);
	
	updateSession($mysqli);
	break;
	
case "reportNoSession":

	// Store APM
	$player = $_REQUEST["player"];
	$apm = $_REQUEST["apm"];
	$minute = time();
	$minute = $minute - $minute%60;
	appendAPMminute($mysqli, $player, $apm, $minute);

	reportNoSession($mysqli);
	break;
	
case "updateAPM":
	updateAPM($mysqli);
	break;

case "playerStats":
	playerStats($mysqli);
	break;
default:
	echo "request not recognized: ". $request;
}

exit();

function query($mysqli, $query) {
	$result = $mysqli->query($query);
	if (!$result) {
		echo($mysqli->error .":". $query);
		exit();
	}		
	return $result;
}


function store_array (&$data, $table, $mysqli)
{
	$cols = implode(',', array_keys($data));
	foreach (array_values($data) as $value)
	{
		isset($vals) ? $vals .= ',' : $vals = '';
		$vals .= '\''.$mysqli->real_escape_string($value).'\'';
	}
	query($mysqli, 'REPLACE INTO '.$table.' ('.$cols.') VALUES ('.$vals.')');
}

function appendAPMminute($mysqli, $player, $apm, $minute) {
	if ($apm == 0) {
		// Don't append zeros
		return;
	}
	$result = $mysqli->query("
		SELECT player, began, IFNULL(ends,0) AS ends, apm
		FROM gl_apm_minute
		WHERE player='$player'
		AND ends=(
			SELECT IFNULL(MAX(ends),0)
			FROM gl_apm_minute
			WHERE player='$player'
			)
		");
	// Row will always have a value, even if the player doesn't exist
	$row = $result->fetch_array(MYSQLI_ASSOC);
	if ((!$row) || ($row["ends"] <= $minute)) {		
		// New data
		if (($row["ends"] == $minute) && ($row["ends"]-$row["began"] < 3600)) {
			// We append an existing row
			$mysqli->query("
				UPDATE gl_apm_minute 
				SET apm='". ($row["apm"].",".$apm) ."',
					ends=". ($minute+60) ."
				WHERE 
					player='$player' AND ends=$minute");
		} else {
			// Start a new row		
			$mysqli->query("INSERT INTO gl_apm_minute (player, began, ends, apm) VALUES
				('$player', $minute, ".($minute+60). ", '$apm')");
		}
	}
}


function updateSession($mysqli) {
	
	$gameid = $_REQUEST["gameid"];
	$player = $_REQUEST["player"];
	$apmsum = $_REQUEST["apmsum"];
	$updates = $_REQUEST["updates"];
	
	$result = query($mysqli, "SELECT * FROM gl_playing WHERE player=\"".$player."\"");
	$row = $result->fetch_array(MYSQLI_ASSOC);
	
	$began = time();
	
	if ($row) {
		// Check if there was a too big a gap between previous update
		if (time() - $row["updated"] > 15*60 || $row["gameid"] != $gameid) {
		
			echo "Moving old playing to history.<br>";
			
			// Allow 15 min space within updates before game can not be continued			
			store_array($row, "gl_history", $mysqli);

			// Set up new session
			$apmsum = 0;
			$updates = 0;
		} else {
			$began = $row["began"];		
		}
	} 
	
	query($mysqli, "
		REPLACE INTO gl_playing 
		SET 
			gameid=$gameid,
			apmsum=$apmsum,
			updates=$updates,
			began=$began,			
			updated=".time().",
			player=\"".$player."\"");	

}

function reportNoSession($mysqli) {
	$player = $_REQUEST["player"];
	
	$result = query($mysqli, "SELECT * FROM gl_playing WHERE player=\"".$player."\"");
	$row = $result->fetch_array(MYSQLI_ASSOC);
	
	if ($row) {
		if ($row["updates"] > 0) {
			echo "Moving old playing to history.<br>";
				
			store_array($row, "gl_history", $mysqli);
		}
		
		// Remove playing row
		query($mysqli,"DELETE FROM gl_playing WHERE player=\"$player\"");
	} 	
}

function playerStats($mysqli) {

	// Per-game totals for a single player, ranked by time played (descending).
	// Time played is the summed span of every session (finished sessions live in
	// gl_history, the ongoing one in gl_playing). UNION ALL keeps both -- there is
	// no overlap between the tables for a given player.
	$player = $mysqli->real_escape_string($_REQUEST["player"]);

	$result = query($mysqli, "
		SELECT gameid,
			SUM(GREATEST(updated - began, 0)) AS seconds,
			COUNT(*) AS sessions,
			MAX(updated) AS lastPlayed
		FROM (
			SELECT gameid, began, updated FROM gl_history  WHERE player='$player'
			UNION ALL
			SELECT gameid, began, updated FROM gl_playing WHERE player='$player'
		) sessions
		GROUP BY gameid
		ORDER BY seconds DESC");

	$stats = array();
	while ($row = $result->fetch_array(MYSQLI_ASSOC)) {
		array_push($stats, array(
			"gameid"     => intval($row["gameid"]),
			"seconds"    => intval($row["seconds"]),
			"sessions"   => intval($row["sessions"]),
			"lastPlayed" => intval($row["lastPlayed"])));
	}

	header("Content-Type: application/json");
	echo json_encode($stats);
}

function updateAPM($mysqli) {

	$player = $_REQUEST["player"];	
	
	// Store APM away
	query($mysqli,"INSERT INTO gl_apm_history (player, updated, apm) SELECT * FROM gl_apm WHERE player='$player'");	
	
	query($mysqli, "REPLACE gl_apm(player, updated, apm) VALUES (\"" . $player ."\", " . time() . ", \"" . $_REQUEST["apm"]."\")");	

}

?>