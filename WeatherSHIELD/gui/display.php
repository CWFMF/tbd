<?php
    $config = parse_ini_file("settings.ini", true);
?>
<!DOCTYPE html>
<html lang="en">
<?php
require 'connection.php';
$SIMPLE_FUELS = array('C1', 'C2', 'C3', 'C4', 'C5', 'C6', 'C7', 'D1', 'D2', 'S1', 'S2', 'S3');
$GRASS_FUELS = array('O1A25', 'O1A50', 'O1A75', 'O1A100', 'O1B25', 'O1B50', 'O1B75', 'O1B100');
$PERCENT_FUELS = array('M125', 'M150', 'M175', 'M225', 'M250', 'M275', 'M330', 'M360', 'M3100', 'M430', 'M460', 'M4100');
$ALL_FUELS = array_merge($SIMPLE_FUELS, $GRASS_FUELS, $PERCENT_FUELS);

/* Log the request to the database */
$is_debug= isset($_GET["debug"]) ? $_GET['debug'] : False;
$lat = isset($_GET["y"]) ? floatval($_GET['y']) : floatval($_GET['txtLATDeg']);
$long = isset($_GET["x"]) ? floatval($_GET['x']) : floatval($_GET['txtLONGDeg']);
$offset = intval($_GET['dateOffset']);
if (isset($_GET['s']))
{
    $fixedStart= date_create_from_format('Y-m-d', $_GET['s']);
    $timezone = new DateTimeZone('UTC');
    $cur_date = new DateTime('midnight', $timezone);
    $fixedStart->setTimeZone($timezone);
    # unsure why but this seems to work
    $fixedStart->modify('-24 hours');
    $offset = $cur_date ->diff($fixedStart);
    $offset = intval($offset->days);
    if ($fixedStart <$cur_date)
    {
        $offset = -$offset;
    }
}
$numDays = isset($_GET["d"]) ? intval($_GET['d']) : intval(test_input($_GET["numDays"]));
if (!$numDays)
{
    $numDays = 30;
}
$show_scenarios = isset($_GET["Scenarios"]) ? 'true' : 'false';
$override = isset($_GET["override"]) ? 'true' : 'false';
$query = $_SERVER['QUERY_STRING'];
$conn = connect('WXSHIELD');
/* Close the connection. */
pg_close( $conn);

//define variables and set them to false or empty
$strLat = "";
$strLong = "";
date_default_timezone_set('America/New_York');
$reportDate = date("Y-m-d Hi");
// NOTE: assuming that boolean values are only set if they are true
// set these to value of 'all' fields so that if we add indices later then
// url will turn them on
// if nothing set then show everything
$weatherAll = isset($_GET["weatherAll"]);
$indicesAll = isset($_GET["IndiciesAll"]);
$fbpAll = isset($_GET["fbpAll"]);
if (!(isset($_GET["weatherAll"])
    || isset($_GET["IndiciesAll"])
    || isset($_GET["fbpAll"])
    || isset($_GET["APCP"])
    || isset($_GET["TMP"])
    || isset($_GET["RH"])
    || isset($_GET["WS"])
    || isset($_GET["WD"])
    || isset($_GET["FFMC"])
    || isset($_GET["DMC"])
    || isset($_GET["DC"])
    || isset($_GET["ISI"])
    || isset($_GET["BUI"])
    || isset($_GET["FWI"])
    || isset($_GET["dateOffset"])
    || isset($_GET["numDays"])))
{
    $any_fuel = false;
    foreach ($ALL_FUELS as $f)
    {
        $any_fuel = $any_fuel || isset($_GET[$f]);
    }
    if (!$any_fuel)
    {
        $weatherAll = true;
        $indicesAll = true;
    }
}
$indices = array();
$show_indices = array();
$calc_indices = array();

function set_fuel($fuel) {
    global $indices;
    global $calc_indices;
    global $show_indices;
    $indices['TMP'] = true;
    $indices['RH'] = true;
    $indices['WS'] = true;
    $indices['APCP'] = true;
    $calc_indices['FFMC'] = true;
    $calc_indices['DMC'] = true;
    $calc_indices['DC'] = true;
    $calc_indices['ISI'] = true;
    $calc_indices['BUI'] = true;
    $calc_indices[$fuel] = true;
    $show_indices[$fuel] = true;
}

if ($_SERVER["REQUEST_METHOD"] == "GET") {
    $fire = isset($_GET["f"]) ? test_input($_GET['f']) : (isset($_GET["fire"]) ? test_input($_GET["fire"]) : NULL);
    $wxstn = isset($_GET["wxStn"]) ? test_input($_GET["wxStn"]) : NULL;
    $strLat = $lat;
    $strLong= $long;
    $strLatLong = $strLat. ' x ' .$strLong;
    $cur_date = new DateTime('13:00');
    $cur_date->modify($offset.' days');
    $startDate = $cur_date->format('Y-m-d');
    //Because we are counting today as a day we need to minus one to
    //make the total range correct
    $tempNum = $numDays - 1;
    $cur_date->modify($tempNum. " days");
    $endDate = $cur_date->format('Y-m-d');
    // set $indices keys based on what indices are needed to calculate values
    if($weatherAll || isset($_GET["APCP"])){
            $indices['APCP'] = true;
            $show_indices['APCP'] = true;
            $calc_indices['Accum'] = true;
            $show_indices['Accum'] = true;
            $calc_indices['MinPCP'] = true;
            $calc_indices['AccumMinPCP'] = true;
    }
    if($weatherAll || isset($_GET["TMP"])){
            $indices['TMP'] = true;
            $show_indices['TMP'] = true;
    }
    if($weatherAll || isset($_GET["RH"])){
            $indices['RH'] = true;
            $show_indices['RH'] = true;
    }
    if($weatherAll || isset($_GET["WS"])){
            $indices['WS'] = true;
            $show_indices['WS'] = true;
    }
    if($weatherAll || isset($_GET["WD"])){
            $show_indices['WD'] = true;
            $indices['WD'] = true;
            // need to have WS to do WD graph
            $indices['WS'] = true;
    }
    // NOTE: javascript expects that these indices will appear in this order when they appear
    if($indicesAll ||isset($_GET["FFMC"])){
            $indices['TMP'] = true;
            $indices['RH'] = true;
            $indices['WS'] = true;
            $indices['APCP'] = true;
            $calc_indices['FFMC'] = true;
            $show_indices['FFMC'] = true;
    }
    if($indicesAll ||isset($_GET["DMC"])){
            $indices['TMP'] = true;
            $indices['RH'] = true;
            $indices['APCP'] = true;
            $calc_indices['DMC'] = true;
            $show_indices['DMC'] = true;
    }
    if($indicesAll || isset($_GET["DC"])){
            $indices['TMP'] = true;
            $indices['APCP'] = true;
            $calc_indices['DC'] = true;
            $show_indices['DC'] = true;
    }
    if($indicesAll || isset($_GET["ISI"])){
            $indices['TMP'] = true;
            $indices['RH'] = true;
            $indices['WS'] = true;
            $indices['APCP'] = true;
            $calc_indices['FFMC'] = true;
            $calc_indices['ISI'] = true;
            $show_indices['ISI'] = true;
    }
    if($indicesAll || isset($_GET["BUI"])){
            $indices['TMP'] = true;
            $indices['RH'] = true;
            $indices['APCP'] = true;
            $calc_indices['DMC'] = true;
            $calc_indices['DC'] = true;
            $calc_indices['BUI'] = true;
            $show_indices['BUI'] = true;
    }
    if($indicesAll || isset($_GET["FWI"])){
            $indices['TMP'] = true;
            $indices['RH'] = true;
            $indices['WS'] = true;
            $indices['APCP'] = true;
            $calc_indices['FFMC'] = true;
            $calc_indices['DMC'] = true;
            $calc_indices['DC'] = true;
            $calc_indices['ISI'] = true;
            $calc_indices['BUI'] = true;
            $calc_indices['FWI'] = true;
            $show_indices['FWI'] = true;
    }
    foreach ($ALL_FUELS as $f)
    {
        if($fbpAll || isset($_GET[$f])){
            set_fuel($f);
        }
    }
  }

function test_input($data) {
    $data = trim($data);
    $data = stripslashes($data);
    $data = htmlspecialchars($data);
    return $data;
}

function makeJSArray($arr) {
return '['.implode(', ', array_map(create_function('$v', 'return "\'".$v."\'";'), array_keys($arr))).']';
}

?>
    <title>WeatherSHIELD</title>
    <link rel="icon" href="favicon.png" type="image/vnd.microsoft.icon">
    <link rel="stylesheet" href="css/styles.css">

    <head>
        <script type='text/javascript' src='js/jquery-1.11.3.min.js'></script>
        <script type="text/javascript" src="js/jspdf/jspdf.js"></script>
        <script src="js/DateCalculations.js"> </script>
        <script src="js/calc_fwi.js"></script>
        <script src="js/calc_fbp.js"></script>
        <script src="js/Chart.js/Chart.js"></script>
        <script src="js/WindRose.js"></script>
        <!-- <script src="js/Chart.js/samples/utils.js"></script> -->
        <script src="js/chartjs-plugin-annotation/chartjs-plugin-annotation.js"></script>
        <script src="js/weatherSHIELD_1.js"> </script>

        <script type='text/javascript' src='js/utmconv.js'></script>
        <script type='text/javascript'>
        var ACTIVE_OFFER = "<?php echo($config['FireGUARD']['active_offer']) ?> <?php echo($config['FireGUARD']['email']) ?>";
        var isDebug = "<?php echo $is_debug ?>";
        var strLatLong = "<?php echo $strLatLong ?>";
        var dateOffset = <?php echo $offset ?>;
        var strStartDate= "<?php echo $startDate ?>";
        var strEndDate = "<?php echo $endDate ?>";
        var numDays = "<?php echo $numDays ?>";
        var strReportDate = "<?php echo $reportDate ?>";
        var theLat = <?php echo $strLat ?>;
        var theLong = <?php echo $strLong ?>;
        var mapDatum = 1;
        var theFire = <?php echo ($fire) ? ("'" . $fire . "'") : "null" ?>;
        utmconv.setDatum(mapDatum);
        var coords = utmconv.latLngToUtm(theLat, theLong);
        var strZone = coords.global.zone;
        var strBaseMap = coords.global.easting.toString().substring(0,2)+coords.global.northing.toString().substring(0,3);
        var requestIndices = "<?php echo implode(',', array_keys($indices)) ?>";
        var calcIndices = <?php echo makeJSArray($calc_indices) ?>;
        var showIndices = <?php echo makeJSArray($show_indices) ?>;
        var showScenarios = <?php echo $show_scenarios ?>;
        var overrideFWI = <?php echo $override ?>;
        var manualStartup = {
            'FFMC': <?php echo (isset($_GET["setFFMC"]) ? floatval($_GET['setFFMC']) : 'null') ?>,
            'DMC': <?php echo (isset($_GET["setDMC"]) ? floatval($_GET['setDMC']) : 'null') ?>,
            'DC': <?php echo (isset($_GET["setDC"]) ? floatval($_GET['setDC']) : 'null') ?>,
            'APCP_0800': <?php echo (isset($_GET["setAPCP_0800"]) ? floatval($_GET['setAPCP_0800']) : 'null') ?>
        };
        $(function () {
            $('#zone').text(strZone);
            $('#basemap').text(strBaseMap);
            loadAll();
        });
        </script>
    <style>
    canvas {
        -moz-user-select: none;
        -webkit-user-select: none;
        -ms-user-select: none;
    }
    </style>
    </head>
    <body>

    <div align='center'>
        <h1 id='title' style='color:black'>WeatherSHIELD Report</h1>
        <table>
            <tr>
                <td class='report'>
                    <img id='imgLogo' src='img/weathershield.png' alt='WeatherSHIELD Logo' style='width:auto; height: 150px'>
                </td>
                <td class='report'>
                    <p id='reportTitle'>
                        <?php if($fire) echo $fire."<br />" ?>
                        <?php if($wxstn) echo 'Weather Station: '.$wxstn."<br />" ?>
                        Location: Zone <span id='zone'></span>  <span id='basemap'></span> (<?php echo $strLatLong  ?>)<br>
                        Date Range: <?php echo $startDate  ?> to <?php echo $endDate  ?> <span id='datesAvailable'></span><br>
                        Total of <?php echo $numDays  ?> days <span id='numDaysAvailable'></span><br>
                        Generated Report Time: <?php echo $reportDate  ?> EST<br>
                    </p>
                </td>
                <td class='report'>
                    <?php if($is_debug) echo "<a id='datalink' href=''>" ?>
                    <img id='imgAFFES' src='img/AFFES.png' alt='AFFES Logo' style='width:auto; height: 150px'>
                    <?php if($is_debug) echo "</a>" ?>
                </td>
            </tr>
            <tr>
                <td>
                </td>
                <td>
                    <div style='float: left; width: 33%'>
                        <a id='export_letter' href="#" style="display: none; font-size: 12px;" class='button button-report'>EXPORT&nbsp;(LETTER)</a>
                    </div>
                    <div style='float: left; width: 33%'>
                        <a id='export_tabloid' href="#" style="display: none; font-size: 12px;" class='button button-report'>EXPORT&nbsp;(11X17)</a>
                    </div>
                    <div style='float: left; width: 33%'>
                        <a id='export_csv' href="#" style="display: none; font-size: 12px;" class='button button-report'>EXPORT&nbsp;CSV</a>
                    </div>
                </td>
                <td>
                </td>
            </tr>
        </table>
    <p id='msg'></p>
    </div>
<br />
<span id='noIndices' style='display: none;text-align: center;'>
    <h3>No charts requested - only displaying station/model data</h3>
</span>
<span id='spanTMP' style='display: none;'>
    <div class='chart chart7' style='background-color: white;'>
        <canvas id="TMP" style='width: 100%; height: 100%;'></canvas>
    </div>
    <br>
</span>
<span id='spanRH' style='display: none;'>
    <div class='chart chart1' style='background-color: white;'>
        <canvas id="RH" style='width: 100%; height: 100%;'></canvas>
    </div>
    <br>
</span>
<span id='spanWS' style='display: none;'>
    <div class='chart chart2' style='background-color: white;'>
        <canvas id="WS" style='width: 100%; height: 100%;'></canvas>
    </div>
    <br>
</span>
<div id='spanWD' style='display: none;'>
    <div class='chart3 chartWD' style='background-color: white;'>
        <canvas id='WDDay1' style='width: 100%; height: 100%;'></canvas>
    </div>
    <div class='chart3 chartWD' style='background-color: white;'>
        <canvas id='WDDay2' style='width: 100%; height: 100%;'></canvas>
    </div>
    <div class='chart3 chartWD' style='background-color: white;'>
        <canvas id='WDDay3' style='width: 100%; height: 100%;'></canvas>
    </div>
    <div class='chart3 chartWD' style='background-color: white;'>
        <canvas id='WD' style='width: 100%; height: 100%;'></canvas>
    </div>
    <br>
</div>
<br>
<span id='spanAPCP' style='display: none;'>
    <div class='chart chart4' style='background-color: white;'>
        <canvas id="APCP" style='width: 100%; height: 100%;'></canvas>
    </div>
    <br>
</span>
<span id='spanAccum' style='display: none;'>
    <div class='chart chart4' style='background-color: white;'>
        <canvas id="Accum" style='width: 100%; height: 100%;'></canvas>
    </div>
    <br>
</span>
<span id='spanFFMC' style='display: none;'>
    <div class='chart chart4' style='background-color: white;'>
        <canvas id="FFMC" style='width: 100%; height: 100%;'></canvas>
    </div>
    <br>
</span>
<span id='spanDMC' style='display: none;'>
    <div class='chart chart4' style='background-color: white;'>
        <canvas id="DMC" style='width: 100%; height: 100%;'></canvas>
    </div>
    <br>
</span>
<span id='spanDC' style='display: none;'>
    <div class='chart chart4' style='background-color: white;'>
        <canvas id="DC" style='width: 100%; height: 100%;'></canvas>
    </div>
    <br>
</span>
<span id='spanISI' style='display: none;'>
    <div class='chart chart5' style='background-color: white;'>
        <canvas id="ISI" style='width: 100%; height: 100%;'></canvas>
    </div>
    <br>
</span>
<span id='spanBUI' style='display: none;'>
    <div class='chart chart5' style='background-color: white;'>
        <canvas id="BUI" style='width: 100%; height: 100%;'></canvas>
    </div>
    <br>
</span>
<span id='spanFWI' style='display: none;'>
    <div class='chart chart5' style='background-color: white;'>
        <canvas id="FWI" style='width: 100%; height: 100%;'></canvas>
    </div>
    <br>
</span>
<?php
    foreach ($ALL_FUELS as $f)
    {
        if (isset($_GET[$f]))
        {
            echo "<span id='span".$f."' style='display: none;'>";
            echo "<div class='chart6 chart' style='background-color: white;'><canvas id='".$f."' style='width: 100%; height: 100%;'></canvas>";
            echo "<br /></span>";
        }
    }
?>
<span id='spanHistoricBar' style='display: none;'>
    <div id="container" class='chart chart7' style='background-color: white;'>
        <canvas id="historic" style='width: 100%; height: 100%;'></canvas>
    </div>
</span>
<span id='spanAFFES'>
    <div id='affesHeader' style="color:black"></div>
    <div id='affesInfo' style="color:black"></div>
    <br />
</span>
        <hr>
        <p id='disclaimer' style="color:black" >
            Note:<br />
            WeatherSHIELD is a prototype (<?php require 'version.php' ?>)<br />
            The indicated ranges of forecast values are approximations<br />
            See <a href='https://www.mdpi.com/2571-6255/3/2/16'>Info/About</a> for full Disclaimer<br />
            Statistics do not include AFFES forecast<br />
            Day 1 precip 0800 - 1300 forecast + 0800 observed as indicated by CFFWI station used<br />
            Weather values forecast for 1300;  indices for 1700
        </p>
        <p id='forecastInfo' style="color:black"></p>
        <?php require 'footer.php' ?>

    </body>
</html>