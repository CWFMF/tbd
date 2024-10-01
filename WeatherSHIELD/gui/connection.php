<?php
$WX_DATABASE = 'FireGUARD';
$DFOSS_DATABASE = 'FireGUARD';
// echo phpinfo();
// exit();
function connect($db)
{
    return pg_pconnect( "host=172.18.0.2 port=5432 dbname=FireGUARD user=wx_readonly password=wx_r34d0nly!");
}

function try_query($conn, $sql)
{
	$stmt = pg_query($conn, $sql);
	if( $stmt === false) {
        echo("Error could not run SQL command: ".$sql."\n".print_r( pg_last_error(), true));
        die;
    }
    return $stmt;
}

$conn = connect($DFOSS_DATABASE);
if ($conn){
/* Close the connection. */
pg_close($conn);
}
else{
    echo("Error could not connect to SQL database: ".print_r( pg_last_error(), true));
    exit();
}

?>
