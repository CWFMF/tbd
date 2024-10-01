// // https://stackoverflow.com/questions/951021/what-is-the-javascript-version-of-sleep


// // sleep time expects milliseconds
// function sleep (time) {
//     return new Promise((resolve) => setTimeout(resolve, time));
//   }

// //   // Usage!
// //   sleep(500).then(() => {
// //       // Do something after the sleep!
// //   });

var theLat = 45;
var theLong = -90;

// var dataUrl = "https://app-cwfmf-api-cwfis-dev.wittyplant-59b495b3.canadacentral.azurecontainerapps.io/gribwx?lat=" + theLat + "&lon=" + theLong + "&model=all&format=csv";

// xmlhttp = new XMLHttpRequest();

// xmlhttp.open("GET", dataUrl, true);
// xmlhttp.send();
// xmlhttp.onreadystatechange = function () {
//     if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
//         lines = xmlhttp.responseText.split("\n");
//         console.log(lines);
//     }
// };

function loadData() {
    if (window.XMLHttpRequest) {
        // code for IE7+, Firefox, Chrome, Opera, Safari
        xmlhttp = new XMLHttpRequest();
    } else {
        // code for IE6, IE5
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
    }
    // var dataUrl = "getEnsembles.php?lat="+theLat+"&long="+theLong+"&dateOffset="+dateOffset+"&numDays="+numDays+"&indices="+requestIndices;
    // lat=53.5&lon=-113.5&model=all&format=csv
    var dataUrl = "https://app-cwfmf-api-cwfis-dev.wittyplant-59b495b3.canadacentral.azurecontainerapps.io/gribwx?lat=" + theLat + "&lon=" + theLong + "&model=all&format=csv";
    if (isDebug) {
        $('#datalink').prop('href', dataUrl);
    }
    xmlhttp.open("GET", dataUrl, true);
    xmlhttp.send();
    xmlhttp.onreadystatechange = function () {
        if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
            var multiArray = [];
            try {
                // HACK: this is the default return value from getEnsembles.php when nothing is in database
                multiArray = {
                    "givenStart": "2024-09-24",
                    "FromDatabase": "FireGUARD",
                    "FakeDatabase": "WX_202409",
                    "Indices": ["TMP", "RH", "WS", "WD", "APCP"],
                    "StartupValues": {
                        "Station": null,
                        "Generated": null,
                        "lat": null,
                        "lon": null,
                        "DistanceFrom": null,
                        "FFMC": 0,
                        "DMC": 0,
                        "DC": 0,
                        "APCP_0800": 0
                    },
                    "qry_FCT_Forecast": "SELECT * FROM INPUTS.FCT_Forecast_By_Offset(0, 45, 90, 15) order by model, member, fortime",
                    "Models": [],
                    "qry_FCT_Hindcast_By_Offset": "SELECT * FROM HINDCAST.FCT_Hindcast_By_Offset(0, 45, 90, 15) order by model, member, fortime",
                    "Hindcast": [],
                    "Actuals": [],
                    "ForDates": [],
                    "StartDate": null,
                };
                lines = xmlhttp.responseText.split("\n");
                // dates_by_model = {};
                models = {};
                for (var i = 0; i < lines.length; ++i) {
                    console.log(i);
                    assert(i < lines.length);
                    // if (0 != i && i < lines.length) {
                    line = lines[i];
                    cols = line.split(",");
                    if (0 == i) {
                        indices = cols.slice(2);
                    } else if (0 < line.length) {
                        model = cols[0];
                        member = 0;
                        g = model.match(/^([^\d]+)([\d]+)$/);
                        if (g != null) {
                            model = g[1];
                            member = parseInt(g[2]);
                        }
                        for_model = models[model] || {};
                        for_members = for_model["Members"] || {};
                        for_member = for_members[member] || {};
                        // FIX: worry about UTC later - it will use sytem timezone for now
                        datetime = new Date(Date.parse(cols[1]));
                        date = new Date(datetime.getFullYear(), datetime.getMonth(), datetime.getDay());
                        for_date = for_member[date] || {};
                        // assign actual indices here
                        for_date[datetime.getHours()] = cols.slice(2);
                        // HACK: reassign nested since unsure if original is modified or may not have existed
                        for_member[date] = for_date;
                        for_members[member] = for_member;
                        for_model["Members"] = for_members;
                        models[model] = for_model;
                        // }
                    }
                }
                // HACK: replace indices names with old ones for now
                i_temp = indices.indexOf("TEMP");
                if (-1 != i_temp) {
                    indices[i_temp] = "TMP";
                }
                i_precip = indices.indexOf("PRECIP");
                if (-1 != i_precip) {
                    indices[i_precip] = "APCP";
                }
                I_APCP = indices.indexOf("APCP");
                I_TMP = indices.indexOf("TMP");
                I_RH = indices.indexOf("RH");
                I_WS = indices.indexOf("WS");
                I_WD = indices.indexOf("WD");

                for (var model in models) {
                    for_model = models[model]["Members"];
                    for (var member in for_model) {
                        for_member = for_model[member];
                        for (var date in for_member) {
                            for_date = for_member[date];
                            min_time = Infinity;
                            apcp = 0;
                            for (var hour in for_date) {
                                for_hour = for_date[hour];
                                console.log([model, member, date, for_date, hour, for_hour]);
                                // HACK: assume local time for now
                                if (min_time > (hour - 12)) {
                                    min_time = hour;
                                }
                                // FIX: calculate 24 hour precip based on calendar day for now
                                apcp += for_hour[I_APCP];
                            }
                            // HACK: reassign a single array for the date
                            wx = for_date[min_time];
                            // replace hourly precip with 24 hour total
                            wx[I_APCP] = apcp;
                            for_member[date] = wx;
                        }
                    }
                }
                // convert date strings into actual dates right away
                multiArray['ForDates'] = multiArray['ForDates'].map(function (currentValue, index, array) {
                    return new Date(currentValue);
                });
                var tmpDates = [];
                //~ var date = new Date();
                //~ date = new Date(Date.UTC(date.getUTCFullYear(), date.getUTCMonth(), date.getUTCDate(), 18, 0, 0));
                var date = new Date(multiArray['StartDate']);
                for (var i = 0; i < numDays; i++) {
                    tmpDates[i] = date;
                    date = new Date(date.getTime() + 86400000);
                }
                var duration = multiArray['ForDates'].slice(-1)[0] - multiArray['ForDates'].slice(1)[0];
                duration /= 86400000;
                for (var i in multiArray['ForDates']) {
                    if (multiArray['ForDates'][i].getTime() != tmpDates[i].getTime()) {
                        var x = false;
                    }
                    if (i > 0) {
                        var dateDiff = multiArray['ForDates'][i].getTime() - multiArray['ForDates'][i - 1].getTime();
                        if (dateDiff != 86400000) {
                            var x = false;
                        }
                    }
                }
                multiArray['ForDates'] = tmpDates;
                document.getElementById("msg").innerHTML = "";
                var actualStartDate = multiArray['ForDates'][0];
                var actualEndDate = multiArray['ForDates'].slice(-1)[0];
                if (formatDate(actualStartDate) != strStartDate || formatDate(actualEndDate) != strEndDate) {
                    $('#datesAvailable').html('<br />(Data available for ' + formatDate(actualStartDate) + ' to ' + formatDate(actualEndDate) + ')');
                    $('#numDaysAvailable').html('<br />(' + (dateDifference(actualStartDate, actualEndDate) + 1) + ' days available)');
                }
                //document.getElementById("msg").innerHTML = multiArray;
                var dateArray = multiArray['ForDates'].map(function (x) {
                    // HACK: after about 4 weeks, day of week overlaps dates
                    return shortDate(x, 28 >= multiArray['ForDates'].length);
                });

                multiArray = preProcessEnsembles(multiArray, theLat, calcIndices);
                if (!multiArray) {
                    return;
                }
                setSourceInfo(multiArray);
                makeGraphs(showIndices, multiArray, dateArray);
                // HACK: add a message if there is no historic match data
                if (numDays > 15 && !multiArray['Matches']) {
                    document.getElementById("msg").innerHTML = "No historic match data present in the database - please contact administrator";
                }
                $('#export_letter').css('display', 'inline');
                $('#export_tabloid').css('display', 'inline');
                $('#export_csv').css('display', 'inline');

                $("#export_csv").on('click', function (event) {
                    exportCSV.apply(this, [showIndices, multiArray, 'wxshield.csv']);
                    // IF CSV, don't do event.preventDefault() or return false
                    // We actually need this to be a typical hyperlink
                });
            }
            catch (e) {
                document.getElementById("msg").innerHTML = "Error:" + e + " " + xmlhttp.responseText;
            }
        }
    }
}
