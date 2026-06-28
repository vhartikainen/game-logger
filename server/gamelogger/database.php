<?php
// Database connection settings.
// In Docker these come from the environment (see docker-compose.yml); the
// fallback placeholders preserve the original behaviour for manual setups.
$db_host = getenv("DB_HOST") ?: "<server address>";
$db_user = getenv("DB_USER") ?: "<username>";
$db_pass = getenv("DB_PASSWORD") ?: "<password>";
$db_name = getenv("DB_NAME") ?: "<database>";

$mysqli = new mysqli($db_host, $db_user, $db_pass, $db_name) or die("Could not open database.");
?>
