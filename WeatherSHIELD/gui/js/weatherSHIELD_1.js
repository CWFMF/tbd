var MAX_GRADE = 2.1;
var minMaxLineStyle = 'ShortDash';
var climateLineStyle = 'Dot';
var showBoundsByDefault = false;
var showClimateBoundsByDefault = true;
var showScenariosByDefault = false;
var climateLineWidth = 2;
var DATE_BOUNDS_PADDING = 0.2;
var PROB_OPACITY = 0.3;
var PROB_ZINDEX = 10;
var PRECIP_THRESHOLD = 0.5;
var SIMPLE_FUELS = ['C1', 'C2', 'C3', 'C4', 'C5', 'C6', 'C7', 'D1', 'D2', 'S1', 'S2', 'S3'];
var GRASS_FUELS = ['O1A25', 'O1A50', 'O1A75', 'O1A100', 'O1B25', 'O1B50', 'O1B75', 'O1B100'];
var PERCENT_FUELS = ['M125', 'M150', 'M175', 'M225', 'M250', 'M275', 'M330', 'M360', 'M3100', 'M430', 'M460', 'M4100'];
var ALL_FUELS = SIMPLE_FUELS.concat(GRASS_FUELS).concat(PERCENT_FUELS);
// NOTE: needs to include 1.0 so that we get min/max lines properly
var useRanges = [0.66, 0.90, 1.0];

var WIND_COLOURS = [
    '#99ebff',
    '#66ff66',
    '#ffff1a',
    '#ff6600',
    '#e60000'
    ];
var WIND_LABELS = ['0-5', '5-10', '10-15', '15-20', '20+'];

var sourceDetails = {};

// HACK: keep track of which years we can't use because we asked for too many days
var removeYears = {};
var historicBarData = {
    'Excellent': [],
    'Good': [],
    'Fair': [],
    'Marginal': [],
    'Unusable': []
};
var historicCategories = [];
var historicGraphData = {
    'Count': 0,
    'Total': 0
};

var cjs_historicDatasets = {
    'Excellent': [],
    'Good': [],
    'Fair': [],
    'Marginal': [],
    'Unusable': []
};
var cjs_historicLabels = [];
var cjs_historicColors = [];
var cjs_historicBarLabels = [];
var cjs_maxSelected = null;
var rateColour = {
    'Excellent': '#00CC00',
    'Good': '#00FF00',
    'Fair': '#FFFF00',
    'Marginal': '#FFCC00',
    'Unusable': '#7cb5ec'
};

function assert(condition)
{
    if (!condition)
    {
        throw new Error("Assertion failed");
    }
}

// ImplicitMemberIndex and ImplicitDateIndex will not be present if members/dateindex are
// in sequential order starting from 0
// structure for arrayOfEnsembles from getEnsembles.php is in the form:
// {
//     "Indices":[
//         /* LIST OF INDICES REQUESTED */
//         "APCP","TMP","RH","WS","WD"
//     ],
//     "StartupValues":{
//         /* STATION DATA USED AS STARTUP VALUES FOR ALL WEATHER STREAMS */
//         "Station":"SOC",
//         "Generated":"2016-08-28T18:00:00.000Z",
//         "lat":50.7787,
//         "lon":-91.7912,
//         "DistanceFrom":0,
//         "FFMC":63.85,
//         "DMC":6.33,
//         "DC":38.51,
//         "APCP_0800":0
//     },
//     "Models":{
//         "AFFES":{
//             /* BEGIN MODEL DATA */
//             "Generated":"2016-08-29T10:26:00.000Z",
//             "lat":50.81476,
//             "lon":-91.72246,
//             "DistanceFrom":6291,
//             "Members":[
//                 <ImplicitMemberIndex:> [
//                     <ImplicitDateIndex:>
//                         /* LIST OF INDICES REQUESTED IN SAME ORDER AS "Indices" LIST */
//                         3.3,19.3,94,8,230
//                     ],
//                     ...
//                 ],
//                 ...
//             ]
//             /* END MODEL DATA */
//         },
//         "GEFS":{ /* MODEL DATA */ },
//         "GEPS":{ /* MODEL DATA */ }
//     },
//     "Hindcast":{
//         "Reanalysis1v05":{
//             /* MODEL DATA */
//             /* NOTE: Members are numbered by year instead of from 0 - 21 */
//         }
//     },
//     "Matches":{
//         "Order":[
//             /* LIST OF YEARS IN DESCENDING MATCH ORDER */
//             1958,
//             ...
//         ],
//         "Grades":{
//             /* DICTIONARY OF EACH YEAR IN "Order" AS KEY AND GRADE AS VALUE */
//             "1958":0.3446,
//             ...
//         }
//     },
//     "ForDates":[
//         /* LIST OF DATES EACH <ImplicitDateIndex> refers to */
//         "2016-08-29T18:00:00.000Z",
//         ...
//     ]
// }

var CALCULATED_GRADES = {};
var CALCULATED_WEATHER = {};
var SCENARIO_COLOURS = [];
var END_OF_ENSEMBLE = 15;

function processModelMember(curModel, m, latitude, startup, indices, calcIndices, dates)
{
    var curMember = curModel.Members[m];
    // start with startup values
    // if we're already past Sep 15 then the station was started
    var MONTH = dates[0].getMonth() + 1
    var started = (null != startup['Station'] || (MONTH > 9 || (9 == MONTH && 15 <= dates[0].getDate())));
    // station wasn't started if it was shutdown but it is if we're after Sep 15
    var shutdown = !started;
    var lastValues = {
        'Accum': startup['Accum'],
        'AccumMinPCP': startup['AccumMinPCP'],
        'FFMC': startup['FFMC'],
        'DMC': startup['DMC'],
        'DC': startup['DC'],
        'ISI': 0,
        'BUI': 0,
        'FWI': 0
    };
    var i = 0;
    var shutdownDC = null;
    var sinceShutdownAccum = 0.0;
    var makeInput = function()
    {
        var inp = {};
        inp.ffmc = data[indices.indexOf('FFMC')];
        inp.ws = data[indices.indexOf('WS')];
        inp.waz = data[indices.indexOf('WD')];
        inp.ps = 0;
        inp.isi = data[indices.indexOf('ISI')];
        inp.dmc = data[indices.indexOf('DMC')];
        inp.dc = data[indices.indexOf('DC')];
        inp.bui = data[indices.indexOf('BUI')];
        inp.fwi = data[indices.indexOf('FWI')];
        return inp;
    };
    var makeAt = function()
    {
        var at = {};
        at.wsv = data[indices.indexOf('WS')];
        return at;
    };
    var calcFcts = {
        'Accum': function(data) {
                    return data[indices.indexOf('APCP')] + lastValues['Accum'];
                },
        'MinPCP': function(data) {
                    return data[indices.indexOf('APCP')] > PRECIP_THRESHOLD ? 1 : 0;
                },
        'AccumMinPCP': function(data) {
                    return data[indices.indexOf('MinPCP')] + lastValues['AccumMinPCP'];
                },
        'FFMC': function(data) {
                    var result = calc_FFMC(data[indices.indexOf('TMP')],
                            data[indices.indexOf('RH')],
                            data[indices.indexOf('WS')],
                            data[indices.indexOf('APCP')],
                            lastValues['FFMC']);
                    if (shutdown)
                    {
                        // HACK: if we got 2mm of pcp since shutdown then assume 0 indices
                        if (sinceShutdownAccum > 2)
                        {
                            result = 0.0;
                        }
                        else
                        {
                            // want to decrease the value but never increase it
                            result = Math.min(lastValues['FFMC'], result);
                        }
                    }
                    return Math.max(0, result);
                },
        'DMC': function(data) {
                    var result = calc_DMC(data[indices.indexOf('TMP')],
                            data[indices.indexOf('RH')],
                            data[indices.indexOf('APCP')],
                            lastValues['DMC'],
                            MONTH,
                            latitude);
                    if (shutdown)
                    {
                        // HACK: if we got 10mm of pcp since shutdown then assume 0 indices
                        if (sinceShutdownAccum > 10)
                        {
                            result = 0.0;
                        }
                        else
                        {
                            // want to decrease the value but never increase it
                            result = Math.min(lastValues[index], result);
                        }
                    }
                    return Math.max(0, result);
                },
        'DC': function(data) {
                    var result;
                    if (shutdown)
                    {
                        var a = 1.0;
                        // NOTE: concerned about the lack of overwinter precip present in reanalysis data
                        // NOTE: use 0.9 due to deep duff fuels
                        var b = 0.9;
                        var smi_f = 800 * Math.exp(-shutdownDC / 400.0)
                        var smi_s = a * smi_f + b * sinceShutdownAccum;
                        // make sure that the value only ever goes down, but never below 0
                        result = Math.min(lastValues['DC'], 400 * Math.log(800 / smi_s));
                    }
                    else
                    {
                        result = calc_DC(data[indices.indexOf('TMP')],
                                          data[indices.indexOf('APCP')],
                                          lastValues['DC'],
                                          MONTH,
                                          latitude);
                    }
                    return Math.max(0, result);
                },
        'ISI': function(data) {
                    return calc_ISI(data[indices.indexOf('WS')], data[indices.indexOf('FFMC')]);
                },
        'BUI': function(data) {
                    return calc_BUI(data[indices.indexOf('DMC')], data[indices.indexOf('DC')]);
                },
        'FWI': function(data) {
                    return calc_FWI(data[indices.indexOf('ISI')], data[indices.indexOf('BUI')]);
                },
    };
    for (var f in GRASS_FUELS)
    {
        var fuel = GRASS_FUELS[f];
        if (!(fuel in calcFcts))
        {
            lastValues[fuel] = 0.0;
            calcFcts[fuel] = (function (forFuel) { return function(data) {
                    var inp = makeInput();
                    var fuelName = forFuel.substring(0,3).replace('A', 'a').replace('B', 'b');
                    inp.cur = parseInt(forFuel.substring(3, 7));
                    return GetFuel(fuelName).rate_of_spread(inp, makeAt());
                }; })(fuel);
        }
    }
    for (var f in ALL_FUELS)
    {
        var fuel = ALL_FUELS[f];
        if (!(fuel in calcFcts))
        {
            lastValues[fuel] = 0.0;
            calcFcts[fuel] = (function (forFuel) { return function(data) {
                        return GetFuel(forFuel).rate_of_spread(makeInput(), makeAt());
                    }; })(fuel);
        }
    }
    var lastIndex = null;
    for (var dateIndex in curMember) {
        if (lastIndex && lastIndex != (dateIndex - 1))
        {
            // HACK: we have a discontinuous stream, so throw out what's left
            delete curMember[dateIndex];
            continue;
        }
        var data = curMember[dateIndex];
        // need to add in 0800 obs
        if (0 == dateIndex) {
            data[indices.indexOf('APCP')] += startup['APCP_0800'];
        }
        var forDate = new Date(dates[dateIndex]);
        MONTH = forDate.getMonth() + 1;
        // NOTE: need to be able to average at least 3 days
        if (i >= 2) {
            // if we rolled into next year then start checking for startup conditions again
            if (shutdown && started && 1 == MONTH)
            {
                started = false;
            }
            if (!started) {
                // try to see if we can start things
                var sum  = 0;
                for (var j = dateIndex - 2; j <= dateIndex; j++) {
                    sum += curMember[j][indices.indexOf('TMP')]
                }
                var avg = sum / 3;
                // start stations if average temperature is at or over 11C for 3 consecutive days
                started = avg >= 11;
                if (started)
                {
                    // use default startup values for FWI indices
                    // FIX: check that we're doing this right - should these be yesterday's indices (i.e. lastValues) or today's?
                    lastValues['FFMC'] = 85.0;
                    lastValues['DMC'] = 6.0;
                    // FIX: DC adjustment isn't correct over winter
                    if (!shutdownDC)
                    {
                        lastValues['DC'] = 15.0;
                    }
                }
                // we aren't shutdown if we just started
                shutdown = !started;
                if (started)
                {
                    shutdownDC = null;
                    shutdownAccum = null;
                }
            }
            else if (!shutdown && MONTH > 9 || (9 == MONTH && 15 <= forDate.getDate()))
            {
                // try to see if we can stop things
                var sum  = 0;
                for (var j = dateIndex - 2; j <= dateIndex; j++) {
                    sum += curMember[j][indices.indexOf('TMP')]
                }
                var avg = sum / 3;
                // shut down station if DMC < 10 and temperature <= 2.5C for 3 days
                shutdown = 10 > lastValues['DMC'] && 2.5 >= avg;
                // check if past week has been 2.5C
                // NOTE: need to be able to average at least 7 days
                if (!shutdown && i >= 6) {
                    // try to see if we can stop things
                    sum  = 0;
                    for (var j = dateIndex - 6; j <= dateIndex; j++) {
                        sum += curMember[j][indices.indexOf('TMP')]
                    }
                    var avg = sum / 7;
                    // shut down station if temperature <= 2.5C for 7 days
                    shutdown = 2.5 >= avg;
                }
            }
            else if (10 <= MONTH)
            {
                shutdown = true;
            }
        }
        i++;
        // need to do this before calculations so that values use proper accum for today
        if (shutdown)
        {
            sinceShutdownAccum += data[indices.indexOf('APCP')];
        }
        for (var c in calcIndices) {
            var index = calcIndices[c];
            if ('Accum' == index || 'MinPCP' == index || 'AccumMinPCP' == index || started)
            {
                if (0 == dateIndex && ('MinPCP' == index))
                {
                    lastValues[index] = (data[indices.indexOf('APCP')] - startup['APCP_0800']) > PRECIP_THRESHOLD ? 1 : 0;
                }
                else
                {
                    var curValue = calcFcts[index](data);
                    if (shutdown && index == 'DC')
                    {
                        assert(curValue <= lastValues[index]);
                        if (data[indices.indexOf('APCP')] > 0)
                        {
                            assert(0 == curValue || curValue < lastValues[index]);
                        }
                    }
                    lastValues[index] = curValue;
                }
            }
            data.push(lastValues[index]);
        }
        if (started && !shutdown)
        {
            shutdownDC = lastValues['DC'];
            sinceShutdownAccum = 0.0;
        }
        // HACK: remove 0800 APCP from day one after we calculated all the FWI indices
        if (0 == dateIndex) {
            //~ data[indices.indexOf('APCP')] -= startup['APCP_0800'];
            if (-1 != indices.indexOf('Accum')) {
                data[indices.indexOf('Accum')] -= startup['APCP_0800'];
                lastValues['Accum'] = data[indices.indexOf('Accum')];
            }
        }
        // HACK: need to set these all recursively or returned object doesn't have values
        curMember[dateIndex] = data;
        lastIndex = dateIndex;
    }
    return curMember;
}


function processModel(startup, latitude, indices, calcIndices, dates, curModel)
{
    for (var m in curModel.Members) {
        curModel.Members[m] = processModelMember(curModel, m, latitude, startup, indices, calcIndices, dates);
    }
    return curModel;
}

//The ensemble comes in with each line representing the prediction from that weather stream.
function preProcessEnsembles(inputData, latitude, calcIndices){
    //Converting the data to return weather/indicies in seperate arrays
    //each array will be a weather/indicie and each row will represent
    //a single ensembles data points for the date range
    //WeatherArray will be length of 11 (for all the weather options):
    //TMP, RH, WS, WD, APCP, FFMC, DMC, DC, ISI, BUI, FWI
    var output = {};
    
    //~ output['StartupValues'] = inputData['StartupValues'];
    //~ output['Indices'] = inputData['Indices'];
    //~ output['ForDates'] = inputData['ForDates'];
    if (overrideFWI)
    {
        inputData['StartupValues']['Station'] = "Manually Entered";
        inputData['StartupValues']['FFMC'] = manualStartup['FFMC'];
        inputData['StartupValues']['DMC'] = manualStartup['DMC'];
        inputData['StartupValues']['DC'] = manualStartup['DC'];
        inputData['StartupValues']['APCP_0800'] = manualStartup['APCP_0800'];
    }
    var startup = inputData['StartupValues'];
    startup['Accum'] = 0;
    startup['AccumMinPCP'] = 0;
    var indices = inputData['Indices'];
    var dates = inputData['ForDates'];
    var arrayOfEnsembles = inputData['Models'];
    var hindcast = inputData['Hindcast'];
    var observed = inputData['Actuals'];
    var matches = inputData['Matches'];
    // HACK: have to check in this order because otherwise indices might be in wrong order
    var couldCalc = ['Accum', 'MinPCP', 'AccumMinPCP', 'FFMC', 'DMC', 'DC', 'ISI', 'BUI', 'FWI'].concat(ALL_FUELS);
    for (var c in couldCalc)
    {
        var checkIndex = couldCalc[c];
        if (-1 != calcIndices.indexOf(checkIndex)) {
            inputData['Indices'].push(checkIndex);
        }
    }
    
    // need to append dates from successive years
    for (var model in hindcast)
    {
        var curModel = hindcast[model];
        var origLength = curModel.Members[Object.keys(curModel.Members)[0]].length;
        for (var m in curModel.Members)
        {
            var curMember = curModel.Members[m];
            var currentEnd = Object.keys(curMember).slice(-1)[0];
            var offset = 1;
            while (currentEnd < (dates.length - 1) && !(m in removeYears))
            {
                var daysLeft = (dates.length - 1) - currentEnd;
                var f = parseInt(m) + offset;
                if (f in curModel.Members && curModel.Members[f].length == origLength)
                {
                    var fromMember = curModel.Members[f];
                    for (var i = 0; i < Math.min(daysLeft, 365); i++)
                    {
                        curMember[i + (365 * offset)] = fromMember[i].slice();
                    }
                }
                else
                {
                    // FIX: what to do when we go outside possible years?
                    var x = false;
                    // need to remove this year from everything because it goes too far
                    var curYear = (new Date()).getFullYear();
                    for (var i = m; i <= Math.min(f, curYear - 1); i++)
                    {
                        removeYears[i] = true;
                    }
                }
                currentEnd = Object.keys(curMember).slice(-1)[0];
                offset++;
            }
        }
        for (var i in removeYears)
        {
            delete curModel.Members[i];
            // NOTE: don't remove from matches because we still want to show those years in the matches
        }
    }
    
    inputData['Branches'] = {};
    for (var s in arrayOfEnsembles) {
        var curModel = arrayOfEnsembles[s];
        arrayOfEnsembles[s] = processModel(startup, latitude, indices, calcIndices, dates, curModel);
        if (s != 'AFFES')
        {
            if (formatDate(new Date(arrayOfEnsembles[s].Generated)) != formatDate(inputData['ForDates'][0]))
            {
                // we have an ensemble that wasn't generated the day of the query, so it's outdated
                END_OF_ENSEMBLE = Math.min(END_OF_ENSEMBLE, arrayOfEnsembles[s].Members[0].length);
            }
        }
    }
    
    if (matches) {
        var hindcastName = Object.keys(hindcast)[0];
        if (!hindcastName)
        {
            document.getElementById("msg").innerHTML = 'No hindcast data exists - did you load it into the database?';
            return null;
        }
        var fakeHindcast = JSON.parse(JSON.stringify(hindcast[hindcastName]));
        fakeHindcast['Members'] = {};
        for (var s in arrayOfEnsembles) {
            // don't branch from AFFES model
            if (s != 'AFFES' && dates.length > END_OF_ENSEMBLE) {
                var branches = {};
                var sum_grade = 0.0;
                for (var r in matches['Order']) {
                    // key these on the year so that we can keep relevant data together
                    //~ if (-1 == branches.indexOf(h))
                    //~ {
                    var h = matches['Order'][r];
                    // NOTE: compare grade before adding value so that we pick last value that started before MAX_GRADE
                    if (sum_grade < MAX_GRADE && !(h in removeYears)) {
                        // HACK: use abs of grade so that negative years still increase sum
                        sum_grade += Math.abs(matches['Grades'][h]);
                        branches[h] = {};
                        branches[h]['Grade'] = matches['Grades'][h];
                        branches[h]['Data'] = {};
                        // HACK: remove all but the year we want
                        //~ var hYear = JSON.parse(JSON.stringify(hindcast[hindcastName]['Members'][h]));
                        var hYear = hindcast[hindcastName]['Members'][h];
                        // need to tack this on to the end of every member of previous model
                        for (var m in arrayOfEnsembles[s].Members)
                        {
                            //~ var curHindcast = jQuery.extend(true, {}, hindcast[r]);
                            var curHindcast = JSON.parse(JSON.stringify(fakeHindcast));
                            //~ // doesn't work for some reason
                            //~ var curHindcast = fakeHindcast;
                            //~ var curYear = hindcast[hindcastName]['Members'][h];
                            curHindcast['Members'] = {};
                            // HACK: need to pick subset of years from day 15 onward but keep index
                            // HACK: use END_OF_ENSEMBLE since if we didn't load it today we'll have < 15 days
                            var subYear = {};
                            for (var d = END_OF_ENSEMBLE; d < hYear.length; d++)
                            {
                                // HACK: slice to copy values
                                subYear[d] = hYear[d].slice();
                            }
                            curHindcast['Members'][h] = subYear;
                            var curMember = arrayOfEnsembles[s].Members[m];
                            // find last value in the array
                            var fake_startup = {
                                'FFMC': 0,
                                'DMC': 0,
                                'DC': 0,
                                'Station': null
                            };
                            var lastDay = null;
                            var lastDayIndex = null;
                            for (var dateIndex in curMember)
                            {
                                var curDate = dates[dateIndex];
                                if (curDate > lastDay)
                                {
                                    lastDay = curDate;
                                    lastDayIndex = dateIndex;
                                }
                            }
                            
                            var lastData = curMember[lastDayIndex];
                            // HACK: will set more than we need
                            for (var c in couldCalc)
                            {
                                var checkIndex = couldCalc[c];
                                if (-1 != calcIndices.indexOf(checkIndex)) {
                                    fake_startup[checkIndex] = lastData[indices.indexOf(checkIndex)];
                                }
                            }
                            // HACK: fake station here so startup flag gets set
                            if (0 != fake_startup['FFMC']
                                || 0 != fake_startup['DMC']
                                || 0 != fake_startup['DC'])
                            {
                                fake_startup['Station'] = 'FAKE_STATION';
                            }
                            var fake_name = h + 'x' + s + 'x' + m;
                            var curLeaf = processModel(fake_startup, latitude, indices, calcIndices, dates, curHindcast);
                            var curResult = curLeaf['Members'][h];
                            // now splice in calculated values from other streams
                            for (var d = 0; d < END_OF_ENSEMBLE; d++)
                            {
                                //~ curResult[d] = arrayOfEnsembles[s]['Members'][m][d].slice();
                                // NOTE: no need to slice since we won't change these values
                                curResult[d] = arrayOfEnsembles[s]['Members'][m][d];
                            }
                            if (undefined == branches[h]['Data'][hindcastName])
                            {
                                branches[h]['Data'][hindcastName] = curLeaf;
                                // clear out current Members because they're keyed on Year
                                branches[h]['Data'][hindcastName]['Members'] = {};
                            }
                            //~ else
                            //~ {
                            // already have a model in there, so append this as a member of that
                            branches[h]['Data'][hindcastName]['Members'][m] = curResult;
                            //~ }
                        }
                    }
                }
                inputData['Branches'][s] = branches;
            }
        }
    }
    inputData['Models'] = arrayOfEnsembles;
    inputData['Climate'] = {};
    for (var model in hindcast)
    {
        inputData['Climate'][model] = processModel(startup, latitude, indices, calcIndices, dates, hindcast[model]);
    }
    for (var model in observed)
    {
        // HACK: override the first day's precip since APCP_0800 is going to get added in
        var curModel = observed[model];
        for (var m in curModel.Members) {
            var curMember = curModel.Members[m];
            if (data)
            {
                var data = curMember[0];
                // must be startup values or else there wouldn't be observed values
                data[indices.indexOf('APCP')] -= startup['APCP_0800'];
            }
        }
        inputData['Actuals'][model] = processModel(startup, latitude, indices, calcIndices, dates, observed[model]);
    }
    return inputData;
}


function generateEnsembleArray(inputData, ensembleKey, fct, j)
{
    var arr = [];
    var arrayOfEnsembles = inputData[ensembleKey];
    var dates = inputData['ForDates'];
    //Expand to have number of days in each weather input
    for(var i=0; i < dates.length; i++){
        arr[i] = [];
    }
    
    function pushData(year, model, fct)
    {
        for (var m in model.Members) {
            var curMember = model.Members[m];
            for (var dateIndex in curMember) {
                arr[dateIndex].push(fct(year, model, curMember, dateIndex, j));
            }
        }
    }
    
    // NOTE: will be same number of dates as END_OF_ENSEMBLE since we set it to be length of basic models
    if ('Climate' == ensembleKey || dates.length < END_OF_ENSEMBLE || numDays <= 15) {
        for (var s in arrayOfEnsembles) {
            if (s != 'AFFES') {
                pushData(null, arrayOfEnsembles[s], fct);
            }
        }
    }
    else
    {
        var branches = inputData['Branches'];
        if (0 == Object.keys(branches).length)
        {
            // HACK: if nothing to branch into the we need to splice forecasts
            branches = inputData['Models'];
        }
        for (var m in branches) {
            var model = branches[m];
            for (var y in model) {
                var year = model[y];
                for (var h in year['Data']) {
                    //~ pushData(year['Data'][h], year['Grade']);
                    pushData(year, year['Data'][h], fct);
                }
            }
        }
    }
    return arr;
}

function getGrade(year, model, curMember, dateIndex, j)
{
    // all years have the same weight
    if (null == year)
    {
        return 1;
    }
    return year['Grade'];
}

function getValue(year, model, curMember, dateIndex, j)
{
    return curMember[dateIndex][j];
}

function generateGradesArray(inputData, ensembleKey)
{
    return generateEnsembleArray(inputData, ensembleKey, getGrade, null);
}

function processEnsemble(inputData, ensembleKey, forVar) {
    var indices = inputData['Indices'];
    var j = indices.indexOf(forVar);
    if (!(ensembleKey in CALCULATED_WEATHER))
    {
        CALCULATED_WEATHER[ensembleKey] = {};
    }
    // only calculate if we haven't already
    if (!(forVar in CALCULATED_WEATHER[ensembleKey]))
    {
        CALCULATED_WEATHER[ensembleKey][forVar] = generateEnsembleArray(inputData, ensembleKey, getValue, j);
    }
    return CALCULATED_WEATHER[ensembleKey][forVar];
}

function sortData(values, grades)
{
    //~ // HACK: if grades is null then all weights are equal
    //~ if (null == grades)
    //~ {
        //~ values.sort();
        //~ var grades = [];
        //~ for (var i = 0; i < values.length; i++)
        //~ {
            //~ grades.push(1);
        //~ }
        //~ return [values, grades];
    //~ }
    // NOTE: use non-destructive sort so that we can combine ranges later
    var dict_values = {};
    for (var i = 0; i < values.length; i++)
    {
        var value = values[i];
        // HACK: if grades is null then all weights are equal
        var grade = ((null == grades) ? 1.0 : grades[i]);
        if (!(value in dict_values))
        {
            dict_values[value] = [];
        }
        dict_values[value].push(grade);
    }
    var sorted_values = [];
    var sorted_grades = [];
    var sorted_keys = Object.keys(dict_values);
    // these become strings when the turn into keys
    for (var i = 0; i < sorted_keys.length; i++)
    {
        sorted_keys[i] = Number(sorted_keys[i]);
    }
    sorted_keys.sort(compareNumbers);
    var index = 0;
    for (var i = 0; i < sorted_keys.length; i++)
    {
        var value = sorted_keys[i];
        for (var j = 0; j < dict_values[value].length; j++)
        {
            var grade = dict_values[value][j];
            sorted_values.push(value);
            sorted_grades.push(grade);
            index++;
        }
    }
    return [sorted_values, sorted_grades];
}

function findQuantiles(unsorted_values, unsorted_grades, quantiles)
{
    var sorted = sortData(unsorted_values, unsorted_grades);
    var values = sorted[0];
    var grades = sorted[1];
    
    var cum_sum = [];
    var weighted = [];
    var sum_grade = 0.0;
    for (var i = 0; i < grades.length; i++) {
        grade = grades[i];
        sum_grade += grade;
        cum_sum[i] = sum_grade;
    }
    for (var i = 0; i < grades.length; i++) {
        weighted[i] = cum_sum[i];
        weighted[i] /= sum_grade;
    }
    var mean = 0.0;
    var results = [];
    var i = 0;
    for (var q = 0; q < quantiles.length; q++) {
        var quantile = quantiles[q];
        while (i < weighted.length && quantile > weighted[i])
        {
            mean += grades[i] * values[i];
            i++;
        }
        // return the value of the last index that is within the range
        results.push(values[i]);
    }
    // HACK: otherwise only works if we ask for 1.0 quantile
    while (i < weighted.length)
    {
        mean += grades[i] * values[i];
        i++;
    }
    // HACK: only divide once to avoid rounding errors?
    mean /= sum_grade;
    var median = 0;
    var cur_sum = 0;
    var i = 0;
    while (cum_sum[i] < sum_grade / 2)
    {
        i++;
    }
    return [results, mean, values[i]];
}

function compareNumbers(a, b)
{
    return a - b;
}

function fixDigits(value, digits)
{
    return Number(value.toFixed(digits));
}

function makeStats(inputData, ensembleKey, forVar, ranges)
{
    var returnArray = {
        'mean': [],
        'median': [],
        'ranges': []
    };
    // NOTE: passed array will change because of this
    ranges.sort(compareNumbers);
    var quantiles = [];
    for (var i = 0; i < ranges.length; i++)
    {
        quantiles[ranges.length - 1 - i] = 0.5 - (ranges[i] / 2.0);
        quantiles[ranges.length + i] = 0.5 + (ranges[i] / 2.0);
        returnArray['ranges'][i] = [];
    }
    var arrayOfWeather = processEnsemble(inputData, ensembleKey, forVar);
    var arrayOfWeights = CALCULATED_GRADES[ensembleKey];
    //console.log(arrayOfWeather);
    for (var i = 0; i < arrayOfWeather.length; i++)
    {
        // HACK: required because if we go past year end then arrays are empty
        if(arrayOfWeather[i].length > 0)
        {
            var values = arrayOfWeather[i];
            var grades = (('Climate' == ensembleKey) ? null : arrayOfWeights[i]);
            // NOTE: don't need to do values.slice() since findQuantiles() is non-destructive
            var calcs = findQuantiles(values, grades, quantiles);
            var results = calcs[0];
            var mean = calcs[1];
            var median = calcs[2];
            returnArray['mean'][i] = Number(mean.toFixed(1));
            returnArray['median'][i] = Number(median.toFixed(1));
            for (var j = 0; j < results.length; j++)
            {
                results[j] = fixDigits(results[j], 1);
            }
            for (var j = 0; j < ranges.length; j++)
            {
                returnArray['ranges'][j][i] = [results[j], results[results.length - 1 - j]];
            }
        }
        else
        {
            for (var j = 0; j < returnArray['ranges'].length; j++)
            {
                returnArray['ranges'][j][i] = null;
            }
            returnArray['mean'][i] = null;
            returnArray['median'][i] = null;
        }
    }
    
    return returnArray;
}

function getMember(source, member, inputData, forVar){
    return getAnyMember(source, member, inputData, 'Models', forVar);
}

function getAnyMember(source, member, inputData, dataKey, forVar){
    //Converting the data to return weather/indicies controls
    //TMP, RH, WS, WD, APCP, FFMC, DMC, DC, ISI, BUI, FWI
    var indices = inputData['Indices'];
    var dates = inputData['ForDates'];
    var arrayOfEnsembles = inputData[dataKey];
    var curModel = null;
    if (source in arrayOfEnsembles) {
        model = arrayOfEnsembles[source];
    }
    else
    {
        document.getElementById("msg").innerHTML = 'Error loading data for "' + source + '" weather model - please contact support as database is not being populated properly';
        return null;
    }
    return getJustMember(model, member, indices, dates, forVar);
}

function getJustMember(curModel, member, indices, dates, forVar)
{
    var theWeatherArray = [];
    var j = indices.indexOf(forVar);
    for(var i=0; i < dates.length; i++){
        theWeatherArray[i] = null;
    }
    
    if (curModel != null) {
        if (member in curModel.Members) {
            curMember = curModel.Members[member];
            for (var dateIndex in curMember) {
                var data = curMember[dateIndex];
                //console.log("Index: " + dateIndex);
                //for (var j = 0; j < indices.length; j++) {
                // use data length since we don't have FWI indices for climate data
                theWeatherArray[dateIndex] = Number(data[j].toFixed(1));
            }
        }
    }
    return theWeatherArray;
}
function getAFFESForecast(arrayOfEnsembles, forVar){
    return getMember('AFFES', 0, arrayOfEnsembles, forVar);
}

function getClimate(arrayOfEnsembles, forWhat, forVar){
    return getAnyMember('Reanalysis1v05', forWhat, arrayOfEnsembles, 'Climate', forVar);
}

function makeWindrose(WS, WD, arrayOfWeights, startDay, days, tickInterval){
    var windrose = [];
    for(var i = 0; i < 5; i++)
    {
        windrose[i] = [];
        for(var j = 0; j < (360 / tickInterval); j++)
        {
            windrose[i][j] = 0.0;
        }
    }
    if (WS.length === WD.length)
    {
        for(var i=startDay; i < days; i++)
        {
            var sum_grade = 0.0;
            for (var j = 0; j < arrayOfWeights[i].length; j++)
            {
                sum_grade += arrayOfWeights[i][j];
            }
            for(var j = 0; j < WS[i].length; j++)
            {
                var whichDeg = parseInt((WD[i][j] + (tickInterval / 2)) / tickInterval);
                // If we've rolled over into the first interval from before 360 degrees
                if (windrose[0].length == whichDeg)
                {
                    whichDeg = 0;
                }
                // breaks are [0-5, 10, 15, 20, 20+]
                var whichCat = Math.max(0, Math.min(Math.ceil(WS[i][j] / 5) - 1, 4));
                // divide the weight of the value by the total weight for the day and the number of days
                windrose[whichCat][whichDeg] += arrayOfWeights[i][j] / (1.0 * sum_grade * (days - startDay));
            }
        }
    }
    /*
     * Loop through the windrose to calculate the % instead of 
     * just the straight counts
     */
    for(var i = 0; i < windrose.length; i++){
        for(var j = 0; j < windrose[i].length; j++){
            /*
             * I wanted to use toFixed(1) here to give decimal points but it wasn't
             * rounding properly so I have used fun math to get this to give a 
             * single decimal
             */
            windrose[i][j] = Math.round(windrose[i][j] * 1000) / 10;
        }
    }
    return windrose;
}


/*Take the ensembles and go through the source field to display 
 * the date information.  This will inform the user if the data
 * is out of date.
 * 
 * @param {type} arrayOfEnsembles
 * @returns {getSourceDetails.arraySourceDates}
 */
function getSourceDetails(inputData){
    var arraySourceDates = {
        Source: [],
        theDate: [],
        Members: [],
        Latitude: [],
        Longitude: [],
        DistanceFrom: [],
        Notes: []
    };
    var indices = inputData['Indices'];
    var arrayOfEnsembles = inputData['Models'];
    var sourceCount = 0;
    for (var s in arrayOfEnsembles)
    {
        // every item in sources is a Source that has a number of days associated with it
        arraySourceDates.Source[sourceCount] = s;
        arraySourceDates.theDate[sourceCount] = new Date(arrayOfEnsembles[s].Generated);
        arraySourceDates.Latitude[sourceCount] = arrayOfEnsembles[s].lat;
        arraySourceDates.Longitude[sourceCount] = arrayOfEnsembles[s].lon;
        arraySourceDates.DistanceFrom[sourceCount] = arrayOfEnsembles[s].DistanceFrom;
        arraySourceDates.Notes[sourceCount] = '';
        var notes = [];
        // check if forecast is generated on the same UTC date as the first day requested
        if (compareDates(arraySourceDates.theDate[sourceCount], inputData['ForDates'][0]))
        {
            arraySourceDates.Notes[sourceCount] = "Outdated forecast";
        }
        
        //HACK: the way the JSON encoding works when the length is 0 then it's not an array
        //otherwise would use:
        //arraySourceDates.Members[sourceCount] = arrayOfEnsembles[s].Members.length;
        var c = 0;
        for (var m in arrayOfEnsembles[s].Members) {
            c++;
        }
        arraySourceDates.Members[sourceCount] = c;
        sourceCount++;
    }
    return arraySourceDates;
}

function fixAlpha(value, alpha, bgValue)
{
    if (!bgValue)
    {
        bgValue = 255;
    }
    return Math.round(((1.0 - alpha) * bgValue) + (alpha * value));
}


function makeRGBA(red, green, blue, alpha)
 {
    return 'rgba(' + red + ', ' + green + ', ' + blue + ', ' + alpha + ')';
}

function makeRGBFromA(red, green, blue, alpha)
 {
    // create an rgb colour by mixing the alpha value ourself so that it's not transparent at all
    return 'rgb(' + fixAlpha(red, alpha) + ', ' + fixAlpha(green, alpha) + ', ' + fixAlpha(blue, alpha) + ')';
}

function gradeOpacity(value)
{
    // NOTE: equation picked so 0.66 => 0.37 and 0.9 => 0.15
    return Math.min(1.0, Math.max(0.975 - 0.916667 * value, 0));
}

function makeData(multiArray, forVar, red, green, blue)
{
    var weatherArray = makeStats(multiArray, 'Models', forVar, useRanges);
    // only care about min/max and mean for climate
    var climateArray = makeStats(multiArray, 'Climate', forVar, [1.0]);
    var lineColor = makeRGBA(red, green, blue, 1.0);
    var boundsColor = makeRGBA(red, green, blue, 0.0);
    var result = [];
    var forecastAFFES = getAFFESForecast(multiArray, forVar);
    if (null != forecastAFFES)
    {
        result.push({
                    name: 'AFFES',
                    data: forecastAFFES,
                    zIndex: useRanges.length + 4,
                    marker: {
                        fillColor: lineColor,
                        lineWidth: 1,
                        lineColor: '#000000'
                    },
                    color: '#000000'
                });
    }
    if ('Observed' in multiArray['Actuals'])
    {
        var obsStation = Object.keys(multiArray['Actuals']['Observed']['Members'])[0];
        var observed = getAnyMember('Observed', obsStation, multiArray, 'Actuals', forVar);
        if (null != observed)
        {
            result.push({
                        name: 'Observed',
                        data: observed,
                        zIndex: useRanges.length + 4,
                        marker: {
                            symbol: 'diamond',
                            radius: 3,
                            fillColor: lineColor,
                            lineWidth: 1,
                            lineColor: '#333333'
                        },
                        dashStyle: 'LongDash',
                        color: '#333333'
                    });
        }
    }
    // -1 since we want to omit the min/max
    for (var j = 0; j < useRanges.length - 1; j++)
    {
        var useColor = makeRGBFromA(red, green, blue, gradeOpacity(useRanges[j]));
        result.push({
                showInLegend: true,
                name: (useRanges[j] * 100).toFixed(0) + 'th percentile',
                data: weatherArray['ranges'][useRanges.length - j - 1],
                type: 'arearange',
                lineWidth: 0,
                color: useColor,
                fillColor: useColor,
                zIndex: useRanges.length - j - 1
            });
    }
    result.push(
            {
                // HACK: do this so that we can have the line in the legend but treat it as a range
                showInLegend: true,
                visible: showBoundsByDefault,
                name: 'Min/Max',
                marker:{
                    enabled:false
                },
                dashStyle: minMaxLineStyle,
                lineWidth: 0.7,
                color: lineColor,
                zIndex: 0
            });
    result.push({
                showInLegend: false,
                visible: showBoundsByDefault,
                name: 'Min/Max',
                data: weatherArray['ranges'][0],
                linkedTo: ':previous',
                type: 'arearange',
                dashStyle: minMaxLineStyle,
                lineWidth: 0.7,
                color: lineColor,
                fillOpacity: 0.0,
                fillColor: boundsColor,
                zIndex: 0
            });
    /*result.push({
                showInLegend: true,
                name: 'Mean',
                data: weatherArray['mean'],
                marker:{
                    enabled:false
                },
                lineWidth: 2,
                color: lineColor,
                zIndex: useRanges.length + 2
            });*/
    result.push({
                showInLegend: true,
                name: 'Median',
                data: weatherArray['median'],
                marker:{
                    enabled:false
                },
                lineWidth: 3,
                dashStyle: 'ShortDash',
                color: lineColor,
                zIndex: useRanges.length + 2
            });
    result.push({
                showInLegend: true,
                name: 'Climate Min/Max',
                visible: showClimateBoundsByDefault,
                marker:{
                    enabled:false
                },
                dashStyle: climateLineStyle,
                lineWidth: 3,
                color: lineColor,
                zIndex: useRanges.length + 1
            });
    result.push({
                showInLegend: false,
                name: 'Climate Min/Max',
                visible: showClimateBoundsByDefault,
                linkedTo: ':previous',
                data: climateArray['ranges'][0],
                type: 'arearange',
                dashStyle: climateLineStyle,
                lineWidth: climateLineWidth,
                color: lineColor,
                fillOpacity: 0.0,
                fillColor: boundsColor,
                zIndex: useRanges.length + 1
            });
    /*result.push({
                name: 'Climate Mean',
                data: climateArray['mean'],
                marker:{
                    enabled:false
                },
                dashStyle: climateLineStyle,
                lineWidth: climateLineWidth,
                color: '#000000',
                zIndex: useRanges.length + 3
            });*/
    result.push({
                name: 'Climate Median',
                data: climateArray['median'],
                marker:{
                    enabled:false
                },
/*                dashStyle: 'ShortDash',
                lineWidth: climateLineWidth,
*/
                dashStyle: climateLineStyle,
                lineWidth: climateLineWidth,
                color: '#000000',
                zIndex: useRanges.length + 3
            });
    if (showScenarios)
    {
        // add in scenarios lines
        var indices = multiArray['Indices'];
        var dates = multiArray['ForDates'];
        var ensembleData = {};
        
        var wx_data = CALCULATED_WEATHER['Models'][forVar];
        // for every scenario in the stream
        //~ var numberStreams = (wx_data.length >= 15 ? wx_data[15].length: wx_data[0].length);
        var numberStreams = wx_data[0].length;
        // generate colours once so that they match between graphs
        while (SCENARIO_COLOURS.length < numberStreams)
        {
            SCENARIO_COLOURS.push(randomColor());
        }
        var curColor = SCENARIO_COLOURS[0];
        var scenariosWidth = 0.5;
        result.push({
                    name: 'Scenarios',
                    visible: showScenariosByDefault,
                    zIndex: useRanges.length + 1,
                    marker:{
                        enabled:false
                    },
                    lineWidth: scenariosWidth,
                    color: curColor
                });
        var calculated_data = [];
        for (var i = 0; i < numberStreams; i++)
        {
            // need to pick the i'th value of every day from this array
            var data = [];
            for (var d = 0; d < wx_data.length; d++)
            {
                data.push(Number(wx_data[d][i].toFixed(1)));
            }
            calculated_data.push(data);
            result.push({
                name: 'Scenario ' + (i + 1),
                visible: showScenariosByDefault,
                data: data,
                linkedTo: ':previous',
                zIndex: useRanges.length + 1,
                marker:{
                    enabled:false
                },
                lineWidth: scenariosWidth,
                color: curColor
            });
            curColor = SCENARIO_COLOURS[i];
        }
    }
    return result;
}

function makeCJSData(multiArray, forVar, red, green, blue)
{
    var weatherArray = makeStats(multiArray, 'Models', forVar, useRanges);
    var datasets = [];
    var climateArray = makeStats(multiArray, 'Climate', forVar, [1.0]);
    var lineColor = makeRGBA(red, green, blue, 1.0);
    var boundsColor = makeRGBA(red, green, blue, 0.0);
    var minValue = Infinity;
    var maxValue = -Infinity;
    var lower = [];
    var upper = [];
    var r = weatherArray['ranges'][0];
    for (var i in r)
    {
        minValue = Math.min(minValue, r[i][0]);
        maxValue = Math.max(maxValue, r[i][1]);
        lower.push(r[i][0]);
        upper.push(r[i][1]);
    }
    datasets.push({
        label: 'Min/Max',
        fill: false,
        pointRadius: 0,
        tension: 0,
        backgroundColor: lineColor,
        borderColor: lineColor,
        borderWidth: 1,
        data: lower,
    });
    datasets.push({
        label: 'Min/Max',
        fill: false,
        pointRadius: 0,
        tension: 0,
        backgroundColor: lineColor,
        borderColor: lineColor,
        borderWidth: 1,
        data: upper,
    });
    if (showScenarios)
    {
        // add in scenarios lines
        var indices = multiArray['Indices'];
        var dates = multiArray['ForDates'];
        var ensembleData = {};
        var wx_data = CALCULATED_WEATHER['Models'][forVar];
        // for every scenario in the stream
        //~ var numberStreams = (wx_data.length >= 15 ? wx_data[15].length: wx_data[0].length);
        var numberStreams = wx_data[0].length;
        // generate colours once so that they match between graphs
        while (SCENARIO_COLOURS.length < numberStreams)
        {
            SCENARIO_COLOURS.push(randomColor());
        }
        var curColor = SCENARIO_COLOURS[0];
        var scenariosWidth = 0.5;
        var calculated_data = [];
        for (var i = 0; i < numberStreams; i++)
        {
            // need to pick the i'th value of every day from this array
            var data = [];
            for (var d = 0; d < wx_data.length; d++)
            {
                data.push(Number(wx_data[d][i].toFixed(1)));
            }
            calculated_data.push(data);
            datasets.push({
                label: 'Scenarios',
                fill: false,
                pointRadius: 0,
                tension: 0,
                borderWidth: 1,
                backgroundColor: curColor,
                borderColor: curColor,
                data: data,
            });
            curColor = SCENARIO_COLOURS[i];
        }
    }
    for (var j = 0; j < useRanges.length - 1; j++)
    {
        var useColor = makeRGBFromA(red, green, blue, gradeOpacity(useRanges[j]));
        var r = weatherArray['ranges'][useRanges.length - j - 1]
        lower = [];
        upper = [];
        for (var i in r)
        {
            lower.push(r[i][0]);
            upper.push(r[i][1]);
        }
        datasets.push({
                    label: (useRanges[j] * 100).toFixed(0) + 'th percentile',
                    fill: "+1",
                    pointRadius: 0,
                    tension: 0,
                    backgroundColor: useColor,
                    borderWidth: 0,
                    data: lower,
                });
        datasets.push({
                    label: (useRanges[j] * 100).toFixed(0) + 'th percentile',
                    fill: '-1',
                    pointRadius: 0,
                    tension: 0,
                    backgroundColor: useColor,
                    borderWidth: 0,
                    data: upper,
                });
    }
    datasets.push({
                label: 'Median',
                fill: false,
                pointRadius: 0,
                tension: 0,
                backgroundColor: lineColor,
                borderColor: lineColor,
                borderDash: [5, 5],
                data: weatherArray['median'],
            });
    lower = [];
    upper = [];
    var r = climateArray['ranges'][0];
    for (var i in r)
    {
        minValue = Math.min(minValue, r[i][0]);
        maxValue = Math.max(maxValue, r[i][1]);
        lower.push(r[i][0]);
        upper.push(r[i][1]);
    }
    datasets.push({
        label: 'Climate Min/Max',
        fill: false,
        pointRadius: 0,
        tension: 0,
        backgroundColor: 'black',
        borderColor: 'black',
        borderDash: [1, 5],
        data: lower,
    });
    datasets.push({
        label: 'Climate Min/Max',
        fill: false,
        pointRadius: 0,
        tension: 0,
        backgroundColor: 'black',
        borderColor: 'black',
        borderDash: [1, 5],
        data: upper,
    });
    datasets.push({
        label: 'Climate Median',
        fill: false,
        pointRadius: 0,
        tension: 0,
        backgroundColor: 'black',
        borderColor: 'black',
        borderDash: [1, 5],
        data: climateArray['median'],
    });
    return {
        data: datasets,
        minValue: minValue,
        maxValue: maxValue
    };
}

function randomColor()
{
    return '#'+Math.floor(Math.random()*16777215).toString(16);
    //~ var min = 10;
    //~ var max = 240;
    //~ var range = max - min;
    //~ var r = Math.random() * range + min;
    //~ var g = Math.random() * range + min;
    //~ var b = Math.random() * range + min;
    //~ return '#' + ('0' + Math.floor(r).toString(16)).slice(-2) +
            //~ ('0' + Math.floor(g).toString(16)).slice(-2) +
            //~ ('0' + Math.floor(b).toString(16)).slice(-2);
}

function findProbabilities(multiArray, data, index, threshold, invert, color, zIndex)
{
    var prob_data = [];
    var arrayOfWeather = processEnsemble(multiArray, 'Models', index);
    for (var d in data[0]['data'])
    {
        var count = 0;
        var gtr = 0;
        for (var s in arrayOfWeather[d])
        {
            count++;
            var value = arrayOfWeather[d][s];
            if ((!invert && value > threshold) || (invert  && value <= threshold))
            {
                gtr++;
            }
        }
        prob_data.push(Math.round(gtr / count * 100));
    }
    var prob = {
        name: 'Probability',
        type: 'column',
        yAxis: 1,
        color: color,
        zIndex: zIndex,
        tooltip: {
            valueSuffix: '%',
        }
    };
    prob['data'] = prob_data;
    return prob;
}

function findCJSProbabilities(multiArray, data, index, threshold, invert, color, zIndex)
{
    var prob_data = [];
    var arrayOfWeather = processEnsemble(multiArray, 'Models', index);
    for (var d in data[0]['data'])
    {
        var count = 0;
        var gtr = 0;
        for (var s in arrayOfWeather[d])
        {
            count++;
            var value = arrayOfWeather[d][s];
            if ((!invert && value > threshold) || (invert  && value <= threshold))
            {
                gtr++;
            }
        }
        prob_data.push(Math.round(gtr / count * 100));
    }
    var prob = {
        label: 'Probability',
        type: 'bar',
        yAxisID: "y-axis-1",
        xAxisID: "x-axis-1",
        backgroundColor: color,
    };
    prob['data'] = prob_data;
    return prob;
}
function tooltipPositioner(labelWidth, labelHeight, point) {
    var chart = this.chart;
    var plotLeft = chart.plotLeft;
    var plotWidth = chart.plotWidth;
    var plotRight = plotLeft + plotWidth;
    var tooltipX = point.plotX - labelWidth + plotLeft;
    var tooltipRight = tooltipX + labelWidth;
    if (tooltipX < plotLeft)
    {
        tooltipX = plotLeft;
    }
    else if (tooltipRight > plotRight)
    {
        tooltipX = plotRight - labelWidth;
    }
    var plotTop = chart.plotTop;
    var plotHeight = chart.plotHeight;
    var plotBottom = plotTop + plotHeight;
    var tooltipY = point.plotY;
    var tooltipBottom = tooltipY + labelHeight;
    if (tooltipY < plotLeft)
    {
        tooltipY = plotLeft;
    }
    else if (tooltipBottom > plotBottom)
    {
        tooltipY = plotBottom - labelHeight;
    }
    return {
        x: tooltipX,
        y: tooltipY
    };
}

function makeAccumChart(multiArray, dateArray) {
    var forVar = 'Accum';
    var title = 'To-date Precipitation (Since Day 1 @ 0800 LDT)';
    var red = 0;
    var green = 0;
    var blue = 255;
    var valueSuffix = ' mm';
    var yMin = 0;
    var yMax = null;
    var tickInterval = 5;
    var element = '#' + forVar;
    var axisTitle = 'Total Precipitation (mm)';
    var axis2Title = 'Probability of ' + (invert ? 'No' : '') + ' Precipitation (>' + PRECIP_THRESHOLD + 'mm)';
    var data = makeData(multiArray, forVar, red, green, blue);
    var threshold = .999999;
    var invert = true;
    var opacity = PROB_OPACITY;
    var color = invert ? makeRGBA(255, 0, 0, opacity) : makeRGBA(0, 0, 255, opacity);
    var zIndex = PROB_ZINDEX;
    var accum = findProbabilities(multiArray, data, 'AccumMinPCP', threshold, invert, color, zIndex);
    data.push(accum);
    //~ var data = makeCJSData(multiArray, forVar, red, green, blue);
    //~ var accum = findProbabilities(multiArray, data, 'AccumMinPCP', threshold, invert, color, zIndex);
    var ctx = document.getElementById(forVar).getContext('2d');
    var cjs_data = makeCJSData(multiArray, forVar, red, green, blue);
    var cjs_accum = findCJSProbabilities(multiArray, data, 'AccumMinPCP', threshold, invert, color, zIndex);
    var data = cjs_data['data'];
    // HACK: try to make sure we only have 4 divisions on axis so it lines up with the 25% probabilities
    var yMax = 0;
    var tickInterval = 0;
    while (yMax < cjs_data['maxValue'])
    {
        tickInterval += 10;
        yMax = 4 * tickInterval;
    }
    for (var i = 0; i < data.length; i++)
    {
        //~ data[i]['label'] = 'Accum';
        data[i]['type'] = 'line';
        data[i]['yAxisID'] = "y-axis-0";
    }
    var datasets = data.concat(cjs_accum);
    if (!window.charts)
    {
        window.charts = {};
    }
    window.charts[forVar] = new Chart(ctx, {
        data: {
            labels: dateArray,
            datasets: datasets,
        },
        options: {
            responsive: true,
            title: {
                display: true,
                text: title
            },
            legend: {
                position: 'bottom',
                labels: {
                    useLineStyle: true,
                    filter: function(legendItem, chartData) {
                        // if this is first match then keep it
                        var last = null;
                        for (var i in chartData.datasets)
                        {
                            if (chartData.datasets[i].label == legendItem.text)
                            {
                                return  i == legendItem.datasetIndex;
                            }
                        }
                    }
                },
                onClick: function (e, legendItem) {
                        var ci = window.charts[forVar];
                        var text = legendItem.text;
                        Object.keys(ci.data.datasets).forEach(function (cindex) {
                            var item = ci.data.datasets[cindex];
                            if (item.label == text) {
                                var alreadyHidden = (ci.getDatasetMeta(cindex).hidden === null) ? false : ci.getDatasetMeta(cindex).hidden;
                                ci.data.datasets.forEach(function (e, i) {
                                    var meta = ci.getDatasetMeta(i);
                                    if (i == cindex) {
                                        var alreadyHidden = ci.getDatasetMeta(cindex).hidden === null;
                                        if (!alreadyHidden) {
                                            meta.hidden = null;
                                        } else if (meta.hidden === null) {
                                            meta.hidden = true;
                                        }
                                    }
                                });
                                ci.update();
                            }
                    });
                }
            },
            tooltips: {
                mode: 'index',
                intersect: false,
                callbacks: {
                    label: function(tooltipItem, data) {
                        var text = data.datasets[tooltipItem.datasetIndex].label || '';
                        var label = text;
                        if (label == 'Scenarios')
                        {
                            var i = 0;
                            while (data.datasets[i].label != 'Scenarios')
                            {
                                i++;
                            }
                            var s = tooltipItem.datasetIndex - i + 1;
                            return 'Scenario ' + s + ': ' + Math.round(tooltipItem.yLabel * 100) / 100 + valueSuffix;
                        }
                        if (label) {
                            label += ': ';
                        }
                        label += Math.round(tooltipItem.yLabel * 100) / 100;
                        label += (0 == text.localeCompare('Probability') ? '%' : valueSuffix)
                        for (var i in data.datasets)
                        {
                            var n = parseInt(i);
                            if (n != tooltipItem.datasetIndex && data.datasets[n].label == text)
                            {
                                if (n < tooltipItem.datasetIndex)
                                {
                                    // if this is not the first item that has the label then no tooltip
                                    return '';
                                }
                                label += ' - ' + data.datasets[n].data[tooltipItem.index] + valueSuffix;
                            }
                        }
                        return label;
                    }
                }
            },
            annotation: {
                drawTime: 'afterDatasetsDraw',
                annotations: [{
                        type: 'line',
                        mode: 'vertical',
                        value: dateArray[END_OF_ENSEMBLE - 1],
                        lineWidth: 1,
                        scaleID: 'x-axis-0',
                        borderColor: 'rgba(0, 0, 0, 1)',
                        borderWidth: 1,
                    }]
            },
            hover: {
                mode: 'nearest',
                intersect: true
            },
            scales: {
                xAxes: [{
                    display: true,
                    offset: true,
                    gridLines: {
                        drawOnChartArea: false,
                    },
                    ticks : {
                        minRotation: 45,
                        padding: 10,
                    }
                },
                {
                    display: false,
                    offset: true,
                    gridLines: {
                        drawOnChartArea: false,
                    },
                    ticks : {
                        minRotation: 45,
                        padding: 10,
                    }
                }],
                yAxes: [{
                    display: true,
                    ticks: {
                        //~ min: yMin,
                        max: yMax,
                        //~ autoSkip: true,
                        stepSize: tickInterval,
                    },
                    scaleLabel: {
                        display: true,
                        labelString: axisTitle,
                    },
                    id: "y-axis-0",
                },{
                    display: true,
                    position: 'right',
                    ticks: {
                        //~ min: yMin,
                        max: 100,
                        autoSkip: true,
                        stepSize: 25,
                    },
                    scaleLabel: {
                        display: true,
                        labelString: axis2Title,
                    },
                    id: "y-axis-1",
                }]
            }
        }
    });
    // HACK: turn off Min/Max
    window.charts[forVar].getDatasetMeta(0).hidden = true;
    window.charts[forVar].getDatasetMeta(1).hidden = true;
    for (var i in window.charts[forVar].data.datasets)
    {
        if (window.charts[forVar].data.datasets[i].label == 'Scenarios')
        {
            window.charts[forVar].getDatasetMeta(i).hidden = !showScenariosByDefault;
        }
    }
    window.charts[forVar].update();
}

function makeWxChart(multiArray, dateArray, forVar,
                        title, axisTitle, red, green, blue, valueSuffix, yMin, yMax, tickInterval) {
    var element = '#' + forVar;
    var ctx = document.getElementById(forVar).getContext('2d');
    var cjs_data = makeCJSData(multiArray, forVar, red, green, blue);
    var datasets = cjs_data['data'];
    if (!window.charts)
    {
        window.charts = {};
    }
    window.charts[forVar] = new Chart(ctx, {
        type: 'line',
        data: {
            labels: dateArray,
            datasets: datasets,
        },
        options: {
            responsive: true,
            title: {
                display: true,
                text: title
            },
            legend: {
                position: 'bottom',
                labels: {
                    useLineStyle: true,
                    filter: function(legendItem, chartData) {
                        // if this is first match then keep it
                        var last = null;
                        for (var i in chartData.datasets)
                        {
                            if (chartData.datasets[i].label == legendItem.text)
                            {
                                return  i == legendItem.datasetIndex;
                            }
                        }
                    }
                },
                onClick: function (e, legendItem) {
                        var ci = window.charts[forVar];
                        var text = legendItem.text;
                        Object.keys(ci.data.datasets).forEach(function (cindex) {
                            var item = ci.data.datasets[cindex];
                            if (item.label == text) {
                                var alreadyHidden = (ci.getDatasetMeta(cindex).hidden === null) ? false : ci.getDatasetMeta(cindex).hidden;
                                ci.data.datasets.forEach(function (e, i) {
                                    var meta = ci.getDatasetMeta(i);
                                    if (i == cindex) {
                                        var alreadyHidden = ci.getDatasetMeta(cindex).hidden === null;
                                        if (!alreadyHidden) {
                                            meta.hidden = null;
                                        } else if (meta.hidden === null) {
                                            meta.hidden = true;
                                        }
                                    }
                                });
                                ci.update();
                            }
                    });
                }
            },
            tooltips: {
                mode: 'index',
                intersect: false,
                callbacks: {
                    label: function(tooltipItem, data) {
                        var text = data.datasets[tooltipItem.datasetIndex].label || '';
                        var label = text;
                        if (label == 'Scenarios')
                        {
                            var i = 0;
                            while (data.datasets[i].label != 'Scenarios')
                            {
                                i++;
                            }
                            var s = tooltipItem.datasetIndex - i + 1;
                            return 'Scenario ' + s + ': ' + Math.round(tooltipItem.yLabel * 100) / 100 + valueSuffix;
                        }
                        if (label) {
                            label += ': ';
                        }
                        label += Math.round(tooltipItem.yLabel * 100) / 100 + valueSuffix;
                        for (var i in data.datasets)
                        {
                            var n = parseInt(i);
                            if (n != tooltipItem.datasetIndex && data.datasets[n].label == text)
                            {
                                if (n < tooltipItem.datasetIndex)
                                {
                                    // if this is not the first item that has the label then no tooltip
                                    return '';
                                }
                                label += ' - ' + data.datasets[n].data[tooltipItem.index] + valueSuffix;
                            }
                        }
                        return label;
                    }
                }
            },
            annotation: {
                drawTime: 'afterDatasetsDraw',
                annotations: [{
                        type: 'line',
                        mode: 'vertical',
                        value: dateArray[END_OF_ENSEMBLE - 1],
                        lineWidth: 1,
                        scaleID: 'x-axis-0',
                        borderColor: 'rgba(0, 0, 0, 1)',
                        borderWidth: 1,
                    }]
            },
            hover: {
                mode: 'nearest',
                intersect: true
            },
            scales: {
                xAxes: [{
                    display: true,
                    gridLines: {
                        drawOnChartArea: false,
                    },
                    ticks : {
                        minRotation: 45,
                        padding: 10,
                    }
                }],
                yAxes: [{
                    display: true,
                    ticks: {
                        //~ min: yMin,
                        //~ max: yMax,
                        autoSkip: true,
                        stepSize: tickInterval,
                    },
                    scaleLabel: {
                        display: true,
                        labelString: axisTitle,
                    }
                }]
            }
        }
    });
    // HACK: turn off Min/Max
    window.charts[forVar].getDatasetMeta(0).hidden = true;
    window.charts[forVar].getDatasetMeta(1).hidden = true;
    for (var i in window.charts[forVar].data.datasets)
    {
        if (window.charts[forVar].data.datasets[i].label == 'Scenarios')
        {
            window.charts[forVar].getDatasetMeta(i).hidden = !showScenariosByDefault;
        }
    }
    window.charts[forVar].update();
}

function makePrecipChart(multiArray, dateArray) {
    var forVar = 'APCP';
    var title = '24hr Precipitation (@ 1300 LDT)';
    var red = 0;
    var green = 0;
    var blue = 255;
    var valueSuffix = ' mm';
    var yMin = 0;
    var yMax = null;
    var tickInterval = 5;
    var element = '#' + forVar;
    var data = makeData(multiArray, forVar, red, green, blue);
    var invert = false;
    var opacity = PROB_OPACITY;
    var axisTitle = '24 hr Precipitation (mm)';
    var axis2Title = 'Probability of Precipitation (>0.5mm)';
    //var color = invert ? makeRGBA(255, 0, 0, opacity) : makeRGBA(0, 0, 255, opacity);
    var color = invert ? makeRGBA(255, 0, 0, opacity) : makeRGBA(48, 150, 190, opacity);
    var zIndex = PROB_ZINDEX;
    data.push(findProbabilities(multiArray, data, 'APCP', PRECIP_THRESHOLD, invert, color, zIndex));
    //~ var data = makeCJSData(multiArray, forVar, red, green, blue);
    //~ var accum = findProbabilities(multiArray, data, 'AccumMinPCP', threshold, invert, color, zIndex);
    var ctx = document.getElementById(forVar).getContext('2d');
    var cjs_data = makeCJSData(multiArray, forVar, red, green, blue);
    var cjs_accum = findCJSProbabilities(multiArray, data, 'APCP', PRECIP_THRESHOLD, invert, color, zIndex);
    var data = cjs_data['data'];
    // HACK: try to make sure we only have 4 divisions on axis so it lines up with the 25% probabilities
    var yMax = 0;
    var tickInterval = 0;
    while (yMax < cjs_data['maxValue'])
    {
        tickInterval += 10;
        yMax = 4 * tickInterval;
    }
    for (var i = 0; i < data.length; i++)
    {
        //~ data[i]['label'] = 'Accum';
        data[i]['type'] = 'line';
        data[i]['yAxisID'] = "y-axis-0";
    }
    var datasets = data.concat(cjs_accum);
    if (!window.charts)
    {
        window.charts = {};
    }
    window.charts[forVar] = new Chart(ctx, {
        data: {
            labels: dateArray,
            datasets: datasets,
        },
        options: {
            responsive: true,
            title: {
                display: true,
                text: title
            },
            legend: {
                position: 'bottom',
                labels: {
                    useLineStyle: true,
                    filter: function(legendItem, chartData) {
                        // if this is first match then keep it
                        var last = null;
                        for (var i in chartData.datasets)
                        {
                            if (chartData.datasets[i].label == legendItem.text)
                            {
                                return  i == legendItem.datasetIndex;
                            }
                        }
                    }
                },
                onClick: function (e, legendItem) {
                        var ci = window.charts[forVar];
                        var text = legendItem.text;
                        Object.keys(ci.data.datasets).forEach(function (cindex) {
                            var item = ci.data.datasets[cindex];
                            if (item.label == text) {
                                var alreadyHidden = (ci.getDatasetMeta(cindex).hidden === null) ? false : ci.getDatasetMeta(cindex).hidden;
                                ci.data.datasets.forEach(function (e, i) {
                                    var meta = ci.getDatasetMeta(i);
                                    if (i == cindex) {
                                        var alreadyHidden = ci.getDatasetMeta(cindex).hidden === null;
                                        if (!alreadyHidden) {
                                            meta.hidden = null;
                                        } else if (meta.hidden === null) {
                                            meta.hidden = true;
                                        }
                                    }
                                });
                                ci.update();
                            }
                    });
                }
            },
            tooltips: {
                mode: 'index',
                intersect: false,
                callbacks: {
                    label: function(tooltipItem, data) {
                        var text = data.datasets[tooltipItem.datasetIndex].label || '';
                        var label = text;
                        if (label == 'Scenarios')
                        {
                            var i = 0;
                            while (data.datasets[i].label != 'Scenarios')
                            {
                                i++;
                            }
                            var s = tooltipItem.datasetIndex - i + 1;
                            return 'Scenario ' + s + ': ' + Math.round(tooltipItem.yLabel * 100) / 100 + valueSuffix;
                        }
                        if (label) {
                            label += ': ';
                        }
                        label += Math.round(tooltipItem.yLabel * 100) / 100;
                        label += (0 == text.localeCompare('Probability') ? '%' : valueSuffix)
                        for (var i in data.datasets)
                        {
                            var n = parseInt(i);
                            if (n != tooltipItem.datasetIndex && data.datasets[n].label == text)
                            {
                                if (n < tooltipItem.datasetIndex)
                                {
                                    // if this is not the first item that has the label then no tooltip
                                    return '';
                                }
                                label += ' - ' + data.datasets[n].data[tooltipItem.index] + valueSuffix;
                            }
                        }
                        return label;
                    }
                }
            },
            annotation: {
                drawTime: 'afterDatasetsDraw',
                annotations: [{
                        type: 'line',
                        mode: 'vertical',
                        value: dateArray[END_OF_ENSEMBLE - 1],
                        lineWidth: 1,
                        scaleID: 'x-axis-0',
                        borderColor: 'rgba(0, 0, 0, 1)',
                        borderWidth: 1,
                    }]
            },
            hover: {
                mode: 'nearest',
                intersect: true
            },
            scales: {
                xAxes: [{
                    display: true,
                    offset: true,
                    gridLines: {
                        drawOnChartArea: false,
                    },
                    ticks : {
                        minRotation: 45,
                        padding: 10,
                    }
                },
                {
                    display: false,
                    offset: true,
                    gridLines: {
                        drawOnChartArea: false,
                    },
                    ticks : {
                        minRotation: 45,
                        padding: 10,
                    }
                }],
                yAxes: [{
                    display: true,
                    ticks: {
                        //~ min: yMin,
                        max: yMax,
                        //~ autoSkip: true,
                        stepSize: tickInterval,
                    },
                    scaleLabel: {
                        display: true,
                        labelString: axisTitle,
                    },
                    id: "y-axis-0",
                },{
                    display: true,
                    position: 'right',
                    ticks: {
                        //~ min: yMin,
                        max: 100,
                        autoSkip: true,
                        stepSize: 25,
                    },
                    scaleLabel: {
                        display: true,
                        labelString: axis2Title,
                    },
                    id: "y-axis-1",
                }]
            }
        }
    });
    // HACK: turn off Min/Max
    window.charts[forVar].getDatasetMeta(0).hidden = true;
    window.charts[forVar].getDatasetMeta(1).hidden = true;
    for (var i in window.charts[forVar].data.datasets)
    {
        if (window.charts[forVar].data.datasets[i].label == 'Scenarios')
        {
            window.charts[forVar].getDatasetMeta(i).hidden = !showScenariosByDefault;
        }
    }
    window.charts[forVar].update();
}

function makeWindChart(divId, startDay, endDay, multiArray, dateArray, windroseInterval) {
    var WS = processEnsemble(multiArray, 'Models', 'WS');
    var arrayOfWeights = CALCULATED_GRADES['Models'];
    var WD = processEnsemble(multiArray, 'Models', 'WD');
    // HACK: figure out how many days we actually have
    var days = 0;
    for(var i=0; i < WS.length; i++)
    {
        if (0 < WS[i].length)
        {
            days++;
        }
    }
    var isFull = -1 == endDay;
    if (!isFull)
    {
        days = Math.min(days, endDay);
    }
    var weatherArray = makeWindrose(WS, WD, arrayOfWeights, startDay, days, windroseInterval);
    var numDays = endDay - startDay;
    var title = (isFull || numDays > 1) ? 
                days + ' Day Wind (@ 1300 LDT)'
                : 'Day ' + (startDay + 1) + ' Wind (@ 1300 LDT)';
    var data = [];
    for (var i = 0; i < weatherArray[0].length; i++)
    {
        var d = [];
        for (var j = 0; j < weatherArray.length; j++)
        {
            d.push(weatherArray[j][i]);
        }
        data.push(d);
    }
		var chartColors = window.chartColors;
		var color = Chart.helpers.color;
        var total = 0;
        var max = -Infinity;
        for (var i = 0; i < data.length; i++)
        {
            var cur_max = 0;
            for (var j = 0; j < data[i].length; j++)
            {
                total += data[i][j];
                cur_max += data[i][j];
            }
            max = Math.max(cur_max, max);
        }
//~         for (var i = 0; i < data.length; i++)
//~         {
//~             for (var j = 0; j < data[i].length; j++)
//~             {
//~                 data[i][j] /= total;
//~             }
//~         }
        var config = {
            type: 'windRose',
			data: {
				datasets: [{
					data: data,
				}],
				labels: [
					'0',
					'22.5',
					'45',
					'67.5',
					'90',
                  '112.5',
                  '135',
                  '157.5',
                  '180',
                  '202.5',
                  '225',
                  '247.5',
                  '270',
                  '292.5',
                  '315',
                  '337.5'
				]
			},
			options: {
				responsive: true,
				title: {
					display: true,
					text: [title, 'Wind Speed (km/h)']
				},
				scale: {
                    angleLines: {
                        display: true,
                        
                    },
                      gridLines: {
//~                       display: false,
                        drawOnChartArea: false,
                    },
					ticks: {
                    display: true,
//~ 						beginAtZero: true,
                    max: 1.05 * max,
                        //~ autoSkip: false,
                        callback: function (value, index, values) {
                            return '';
                         },
                    stepSize: 0.8 * max
                    },
					reverse: false
				},
				animation: {
					animateRotate: false,
					animateScale: true
				},
            tooltips: {
                callbacks: {
                labelColor: function(tooltipItem, chart) {
                            return {
                                backgroundColor: ['rgba(0, 0, 0, 0)'].concat(WIND_COLOURS)
                            }
                        },
                    label: function(tooltipItem, data) {
                        var label = [];
                        var d = data.datasets[0].data[tooltipItem.index];
                        var total = 0;
                        for (var i = 0; i < d.length; i++)
                        {
                            total += d[i];
                        }
                        total = Math.round(total * 100) / 100.0;
                        label.push(data.labels[tooltipItem.index] + '\u00B0: ' + total + '%');

                        for (var i = 0; i < WIND_LABELS.length; i++)
                        {
                            label.push(WIND_LABELS[i] + ': ' + d[i] + '%');
                        }
                        return label;
                    }
                }
            },
			legend: {
                position: 'bottom',
                display: true,
                labels: {
                    generateLabels: function(chart) {
                        var data = chart.data;
                        return WIND_LABELS.map(function(label, i) {
                            var meta = chart.getDatasetMeta(0);
                            var style = meta.controller.getStyle(i);

                            return {
                                text: label,
                                fillStyle: WIND_COLOURS[i],
                                strokeStyle: WIND_COLOURS[i],
                                lineWidth: style.borderWidth,
                                hidden: false,

                                // Extra data used for toggling the correct item
                                index: i
                            };
                        });
                        return [];
                    }
                },

                onClick: function(e, legendItem) {
                    // do nothing
                }
            }
        }
    };
    var forVar = divId.replaceAll('#', '');
    var ctx = document.getElementById(forVar);
    if (!window.charts)
    {
        window.charts = {};
    }
    window.charts[forVar] = new Chart(ctx, config);
}


function makeIndex(multiArray, dateArray, forVar,
                        title, yMin, lowMax, modMax, highMax) {
    var element = '#cjs_' + forVar;
    var ctx = document.getElementById(forVar).getContext('2d');
    var cjs_data = makeCJSData(multiArray, forVar, 0, 0, 0);
    var datasets = cjs_data['data'];
    // HACK: need to figure out max y value because otherwise annotation boxes draw weird ranges
    var yMax = cjs_data['maxValue'];
    var valueSuffix = '';
    if (!window.charts)
    {
        window.charts = {};
    }
    window.charts[forVar] = new Chart(ctx, {
        type: 'line',
        data: {
            labels: dateArray,
            datasets: datasets,
        },
        options: {
            responsive: true,
            title: {
                display: true,
                text: title
            },
            legend: {
                position: 'bottom',
                labels: {
                    useLineStyle: true,
                    filter: function(legendItem, chartData) {
                        // if this is first match then keep it
                        var last = null;
                        for (var i in chartData.datasets)
                        {
                            if (chartData.datasets[i].label == legendItem.text)
                            {
                                return  i == legendItem.datasetIndex;
                            }
                        }
                    }
                },
                onClick: function (e, legendItem) {
                        var ci = window.charts[forVar];
                        var text = legendItem.text;
                        Object.keys(ci.data.datasets).forEach(function (cindex) {
                            var item = ci.data.datasets[cindex];
                            if (item.label == text) {
                                var alreadyHidden = (ci.getDatasetMeta(cindex).hidden === null) ? false : ci.getDatasetMeta(cindex).hidden;
                                ci.data.datasets.forEach(function (e, i) {
                                    var meta = ci.getDatasetMeta(i);
                                    if (i == cindex) {
                                        var alreadyHidden = ci.getDatasetMeta(cindex).hidden === null;
                                        if (!alreadyHidden) {
                                            meta.hidden = null;
                                        } else if (meta.hidden === null) {
                                            meta.hidden = true;
                                        }
                                    }
                                });
                                ci.update();
                            }
                    });
                }
            },
            tooltips: {
                mode: 'index',
                intersect: false,
                callbacks: {
                    label: function(tooltipItem, data) {
                        var text = data.datasets[tooltipItem.datasetIndex].label || '';
                        var label = text;
                        if (label == 'Scenarios')
                        {
                            var i = 0;
                            while (data.datasets[i].label != 'Scenarios')
                            {
                                i++;
                            }
                            var s = tooltipItem.datasetIndex - i + 1;
                            return 'Scenario ' + s + ': ' + Math.round(tooltipItem.yLabel * 100) / 100 + valueSuffix;
                        }
                        if (label) {
                            label += ': ';
                        }
                        label += Math.round(tooltipItem.yLabel * 100) / 100 + valueSuffix;
                        for (var i in data.datasets)
                        {
                            var n = parseInt(i);
                            if (n != tooltipItem.datasetIndex && data.datasets[n].label == text)
                            {
                                if (n < tooltipItem.datasetIndex)
                                {
                                    // if this is not the first item that has the label then no tooltip
                                    return '';
                                }
                                label += ' - ' + data.datasets[n].data[tooltipItem.index] + valueSuffix;
                            }
                        }
                        return label;
                    }
                }
            },
            annotation: {
                drawTime: 'beforeDatasetsDraw',
                annotations: [{
                        type: 'line',
                        mode: 'vertical',
                        value: dateArray[END_OF_ENSEMBLE - 1],
                        lineWidth: 1,
                        scaleID: 'x-axis-0',
                        borderColor: 'rgba(0, 0, 0, 1)',
                        borderWidth: 1,
                        drawTime: 'afterDatasetsDraw',
                    },{
                        type: 'box',
                        xScaleID: 'x-axis-0',
                        yScaleID: 'y-axis-0',
                        yMin: 0,
                        yMax: lowMax,
                        backgroundColor: 'rgba(51, 153, 255,0.4)',
                        borderColor: 'rgba(51, 153, 255,0.4)',
                        borderWidth: 1,
                    },{
                        type: 'box',
                        xScaleID: 'x-axis-0',
                        yScaleID: 'y-axis-0',
                        yMin: lowMax,
                        yMax: modMax,
                        backgroundColor: 'rgba(0,255,0,0.4)',
                        borderColor: 'rgba(0,255,0,0.4)',
                        borderWidth: 1,
                    },{
                        type: 'box',
                        xScaleID: 'x-axis-0',
                        yScaleID: 'y-axis-0',
                        yMin: modMax,
                        yMax: highMax,
                        backgroundColor: 'rgba(255,255,0,0.4)',
                        borderColor: 'rgba(255,255,0,0.4)',
                        borderWidth: 1,
                    },{
                        type: 'box',
                        xScaleID: 'x-axis-0',
                        yScaleID: 'y-axis-0',
                        yMin: highMax,
                        backgroundColor: 'rgba(255,0,0,0.4)',
                        borderColor: 'rgba(255,0,0,0.4)',
                        borderWidth: 1,
                    },{
                        type: 'line',
                        mode: 'horizontal',
                        value: (yMin + lowMax) / 2,
                        lineWidth: 0,
                        scaleID: 'y-axis-0',
                        borderColor: 'rgba(0, 0, 0, 0)',
                        borderWidth: 0,
                        label: {
                            backgroundColor: 'rgba(0, 0, 0, 0)',
                            fontColor: '#606060',
                            content: 'Low',
                            enabled: true,
                            position: 'left',
                            xAdjust: -17,
                        },
                    },{
                        type: 'line',
                        mode: 'horizontal',
                        value: (lowMax + modMax) / 2,
                        lineWidth: 0,
                        scaleID: 'y-axis-0',
                        borderColor: 'rgba(0, 0, 0, 0)',
                        borderWidth: 0,
                        label: {
                            backgroundColor: 'rgba(0, 0, 0, 0)',
                            fontColor: '#606060',
                            content: 'Moderate',
                            enabled: true,
                            position: 'left',
                            xAdjust: -17,
                        },
                    },{
                        type: 'line',
                        mode: 'horizontal',
                        value: (modMax + highMax) / 2,
                        lineWidth: 0,
                        scaleID: 'y-axis-0',
                        borderColor: 'rgba(0, 0, 0, 0)',
                        borderWidth: 0,
                        label: {
                            backgroundColor: 'rgba(0, 0, 0, 0)',
                            fontColor: '#606060',
                            content: 'High',
                            enabled: true,
                            position: 'left',
                            xAdjust: -17,
                        },
                    },{
                        type: 'line',
                        mode: 'horizontal',
                        value: (yMax + highMax) / 2,
                        lineWidth: 0,
                        scaleID: 'y-axis-0',
                        borderColor: 'rgba(0, 0, 0, 0)',
                        borderWidth: 0,
                        label: {
                            backgroundColor: 'rgba(0, 0, 0, 0)',
                            fontColor: '#606060',
                            content: 'Extreme',
                            enabled: true,
                            position: 'left',
                            xAdjust: -17,
                        },
                    }]
            },
            hover: {
                mode: 'nearest',
                intersect: true
            },
            scales: {
                xAxes: [{
                    display: true,
                    gridLines: {
                        drawOnChartArea: false,
                    },
                    ticks : {
                        minRotation: 45,
                        padding: 10,
                    }
                }],
                yAxes: [{
                    display: true,
                    ticks: {
                        min: yMin,
                        max: yMax,
                        autoSkip: true,
                        callback: function (value, index, values) {
                            if(value != yMax){
                                return value;
                            }
                        }
                    },
                    scaleLabel: {
                        display: false,
                    }
                }]
            }
        }
    });
    // HACK: turn off Min/Max
    window.charts[forVar].getDatasetMeta(0).hidden = true;
    window.charts[forVar].getDatasetMeta(1).hidden = true;
    for (var i in window.charts[forVar].data.datasets)
    {
        if (window.charts[forVar].data.datasets[i].label == 'Scenarios')
        {
            window.charts[forVar].getDatasetMeta(i).hidden = !showScenariosByDefault;
        }
    }
    window.charts[forVar].update();
}

function makeROS(multiArray, dateArray, forVar, title) {
    var element = '#' + forVar;
    var yMin = 0;
    var yMax = 50;
    var lowMax = 5;
    var modMax = 10;
    var highMax = 20;
    var element = '#cjs_' + forVar;
    var ctx = document.getElementById(forVar).getContext('2d');
    var cjs_data = makeCJSData(multiArray, forVar, 0, 0, 0);
    var datasets = cjs_data['data'];
    //~ // HACK: need to figure out max y value because otherwise annotation boxes draw weird ranges
    //~ var yMax = cjs_data['maxValue'];
    var valueSuffix = '';
    if (!window.charts)
    {
        window.charts = {};
    }
    window.charts[forVar] = new Chart(ctx, {
        type: 'line',
        data: {
            labels: dateArray,
            datasets: datasets,
        },
        options: {
            responsive: true,
            title: {
                display: true,
                text: title
            },
            legend: {
                position: 'bottom',
                labels: {
                    useLineStyle: true,
                    filter: function(legendItem, chartData) {
                        // if this is first match then keep it
                        var last = null;
                        for (var i in chartData.datasets)
                        {
                            if (chartData.datasets[i].label == legendItem.text)
                            {
                                return  i == legendItem.datasetIndex;
                            }
                        }
                    }
                },
                onClick: function (e, legendItem) {
                        var ci = window.charts[forVar];
                        var text = legendItem.text;
                        Object.keys(ci.data.datasets).forEach(function (cindex) {
                            var item = ci.data.datasets[cindex];
                            if (item.label == text) {
                                var alreadyHidden = (ci.getDatasetMeta(cindex).hidden === null) ? false : ci.getDatasetMeta(cindex).hidden;
                                ci.data.datasets.forEach(function (e, i) {
                                    var meta = ci.getDatasetMeta(i);
                                    if (i == cindex) {
                                        var alreadyHidden = ci.getDatasetMeta(cindex).hidden === null;
                                        if (!alreadyHidden) {
                                            meta.hidden = null;
                                        } else if (meta.hidden === null) {
                                            meta.hidden = true;
                                        }
                                    }
                                });
                                ci.update();
                            }
                    });
                }
            },
            tooltips: {
                mode: 'index',
                intersect: false,
                callbacks: {
                    label: function(tooltipItem, data) {
                        var text = data.datasets[tooltipItem.datasetIndex].label || '';
                        var label = text;
                        if (label == 'Scenarios')
                        {
                            var i = 0;
                            while (data.datasets[i].label != 'Scenarios')
                            {
                                i++;
                            }
                            var s = tooltipItem.datasetIndex - i + 1;
                            return 'Scenario ' + s + ': ' + Math.round(tooltipItem.yLabel * 100) / 100 + valueSuffix;
                        }
                        if (label) {
                            label += ': ';
                        }
                        label += Math.round(tooltipItem.yLabel * 100) / 100 + valueSuffix;
                        for (var i in data.datasets)
                        {
                            var n = parseInt(i);
                            if (n != tooltipItem.datasetIndex && data.datasets[n].label == text)
                            {
                                if (n < tooltipItem.datasetIndex)
                                {
                                    // if this is not the first item that has the label then no tooltip
                                    return '';
                                }
                                label += ' - ' + data.datasets[n].data[tooltipItem.index] + valueSuffix;
                            }
                        }
                        return label;
                    }
                }
            },
            annotation: {
                drawTime: 'beforeDatasetsDraw',
                annotations: [{
                        type: 'line',
                        mode: 'vertical',
                        value: dateArray[END_OF_ENSEMBLE - 1],
                        lineWidth: 1,
                        scaleID: 'x-axis-0',
                        borderColor: 'rgba(0, 0, 0, 1)',
                        borderWidth: 1,
                        drawTime: 'afterDatasetsDraw',
                    },{
                        type: 'box',
                        xScaleID: 'x-axis-0',
                        yScaleID: 'y-axis-0',
                        yMin: 0,
                        yMax: lowMax,
                        backgroundColor: 'rgba(51, 153, 255,0.4)',
                        borderColor: 'rgba(51, 153, 255,0.4)',
                        borderWidth: 1,
                    },{
                        type: 'box',
                        xScaleID: 'x-axis-0',
                        yScaleID: 'y-axis-0',
                        yMin: lowMax,
                        yMax: modMax,
                        backgroundColor: 'rgba(0,255,0,0.4)',
                        borderColor: 'rgba(0,255,0,0.4)',
                        borderWidth: 1,
                    },{
                        type: 'box',
                        xScaleID: 'x-axis-0',
                        yScaleID: 'y-axis-0',
                        yMin: modMax,
                        yMax: highMax,
                        backgroundColor: 'rgba(255,255,0,0.4)',
                        borderColor: 'rgba(255,255,0,0.4)',
                        borderWidth: 1,
                    },{
                        type: 'box',
                        xScaleID: 'x-axis-0',
                        yScaleID: 'y-axis-0',
                        yMin: highMax,
                        backgroundColor: 'rgba(255,0,0,0.4)',
                        borderColor: 'rgba(255,0,0,0.4)',
                        borderWidth: 1,
                    },{
                        type: 'line',
                        mode: 'horizontal',
                        value: (yMin + lowMax) / 2,
                        lineWidth: 0,
                        scaleID: 'y-axis-0',
                        borderColor: 'rgba(0, 0, 0, 0)',
                        borderWidth: 0,
                        label: {
                            backgroundColor: 'rgba(0, 0, 0, 0)',
                            fontColor: '#606060',
                            content: 'Low',
                            enabled: true,
                            position: 'left',
                            xAdjust: -17,
                        },
                    },{
                        type: 'line',
                        mode: 'horizontal',
                        value: (lowMax + modMax) / 2,
                        lineWidth: 0,
                        scaleID: 'y-axis-0',
                        borderColor: 'rgba(0, 0, 0, 0)',
                        borderWidth: 0,
                        label: {
                            backgroundColor: 'rgba(0, 0, 0, 0)',
                            fontColor: '#606060',
                            content: 'Moderate',
                            enabled: true,
                            position: 'left',
                            xAdjust: -17,
                        },
                    },{
                        type: 'line',
                        mode: 'horizontal',
                        value: (modMax + highMax) / 2,
                        lineWidth: 0,
                        scaleID: 'y-axis-0',
                        borderColor: 'rgba(0, 0, 0, 0)',
                        borderWidth: 0,
                        label: {
                            backgroundColor: 'rgba(0, 0, 0, 0)',
                            fontColor: '#606060',
                            content: 'High',
                            enabled: true,
                            position: 'left',
                            xAdjust: -17,
                        },
                    },{
                        type: 'line',
                        mode: 'horizontal',
                        value: (yMax + highMax) / 2,
                        lineWidth: 0,
                        scaleID: 'y-axis-0',
                        borderColor: 'rgba(0, 0, 0, 0)',
                        borderWidth: 0,
                        label: {
                            backgroundColor: 'rgba(0, 0, 0, 0)',
                            fontColor: '#606060',
                            content: 'Extreme',
                            enabled: true,
                            position: 'left',
                            xAdjust: -17,
                        },
                    }]
            },
            hover: {
                mode: 'nearest',
                intersect: true
            },
            scales: {
                xAxes: [{
                    display: true,
                    gridLines: {
                        drawOnChartArea: false,
                    },
                    ticks : {
                        minRotation: 45,
                        padding: 10,
                    }
                }],
                yAxes: [{
                    display: true,
                    ticks: {
                        min: yMin,
                        max: yMax,
                        autoSkip: true,
                        callback: function (value, index, values) {
                            if(value != yMax){
                                return value;
                            }
                        }
                    },
                    scaleLabel: {
                        display: false,
                    }
                }]
            }
        }
    });
    // HACK: turn off Min/Max
    window.charts[forVar].getDatasetMeta(0).hidden = true;
    window.charts[forVar].getDatasetMeta(1).hidden = true;
    for (var i in window.charts[forVar].data.datasets)
    {
        if (window.charts[forVar].data.datasets[i].label == 'Scenarios')
        {
            window.charts[forVar].getDatasetMeta(i).hidden = !showScenariosByDefault;
        }
    }
    window.charts[forVar].update();
}


function makeGraphs(showIndices, multiArray, dateArray)
{
    // despite what we've retrieved/caclulated, only show what's asked for
    if (0 == showIndices.length) {
        // didn't ask for anything
        $('#noIndices').css('display', 'block');
    }
    // only calculate once here and then assume they're in the dictionary elsewhere
    CALCULATED_GRADES['Models'] = generateGradesArray(multiArray, 'Models');
    CALCULATED_GRADES['Climate'] = generateGradesArray(multiArray, 'Climate');
    for (var i in showIndices) {
        var index = showIndices[i];
        $('#span' + index).css('display', 'block');
        if ('WD' != index) {
            if ('TMP' == index) {
                makeWxChart(multiArray, dateArray, index,
                            'Temperature (@ 1300 LDT)', 'Temperature (\u00B0C)', 255, 0, 0, ' \u00B0C', null, null, 10);
            }
            else if ('RH' == index) {
                makeWxChart(multiArray, dateArray, index,
                            'Relative Humidity (@ 1300 LDT)', 'Relative Humidity (%)', 255, 0, 255, ' %', 0, 100, 20);
            }
            else if ('APCP' == index) {
                makePrecipChart(multiArray, dateArray);
            }
            else if ('Accum' == index) {
                makeAccumChart(multiArray, dateArray);
            }
            else if ('WS' == index) {
                makeWxChart(multiArray, dateArray, index,
                            'Forecast Wind Speed (10 min avg @ 1300 LDT)', '10 minute average Wind (km/h)',
                            0, 153, 51, 'km/h', 0, null, 10);
            }
            else if ('FWI' == index) {
                makeIndex(multiArray, dateArray, index,
                            'Fire Weather Index (@ 1700 LDT)', 0, 3, 10, 22);
            }
            else if (-1 != SIMPLE_FUELS.indexOf(index)) {
                makeROS(multiArray, dateArray, index, index[0] + '-' + index[1] + ' Rate Of Spread (m/min) (@ 1700 LDT)');
            }
            else if (-1 != GRASS_FUELS.indexOf(index)) {
                makeROS(multiArray, dateArray, index, index[0] + '-' + index.substring(1, 3) + ' ' + index.substring(3, 7) + '% Rate Of Spread (m/min) (@ 1700 LDT)');
            }
            else if (-1 != PERCENT_FUELS.indexOf(index)) {
                makeROS(multiArray, dateArray, index, index[0] + '-' + index[1] + ' ' + index.substring(2, 5) + '% Rate Of Spread (m/min) (@ 1700 LDT)');
            }
            else if ('FFMC' == index) {
                makeIndex(multiArray, dateArray, index,
                            'Fine Fuel Moisture Code (@ 1700 LDT)', 70, 80, 86, 90);
            }
            else if ('DMC' == index) {
                makeIndex(multiArray, dateArray, index,
                            'Duff Moisture Code (@ 1700 LDT)', 0, 15, 30, 50);
            }
            else if ('DC' == index) {
                makeIndex(multiArray, dateArray, index,
                            'Drought Code (@ 1700 LDT)', 0, 140, 240, 340);
            }
            else if ('ISI' == index) {
                makeIndex(multiArray, dateArray, index,
                            'Initial Spread Index (@ 1700 LDT)', 0, 2.2, 5, 10);
            }
            else if ('BUI' == index) {
                makeIndex(multiArray, dateArray, index,
                            'Buildup Index (@ 1700 LDT)', 0, 20, 36, 60);
            }
        }
        else
        {
            // The windrose is a special case compared to the other graphs and thus needs to be processed differently
            var windroseInterval = 22.5;
            //~ $('#spanWD3Day').css('display', 'block');
            makeWindChart('#WDDay1', 0, 1, multiArray, dateArray, windroseInterval);
            makeWindChart('#WDDay2', 1, 2, multiArray, dateArray, windroseInterval);
            makeWindChart('#WDDay3', 2, 3, multiArray, dateArray, windroseInterval);
            makeWindChart('#WD', 0, -1, multiArray, dateArray, windroseInterval);
        }
    }
    makeHistoricChart();
}


function rateGrade(grade) {
    var lowerBound = {
        'Excellent': 0.25,
        'Good': 0.14,
        'Fair': 0.11,
        'Marginal': 0.0
    };
    if (grade >= lowerBound['Excellent']) {
        return 'Excellent';
    }
    else if (grade >= lowerBound['Good']) {
        return 'Good';
    }
    else if (grade >= lowerBound['Fair']) {
        return 'Fair';
    }
    return 'Marginal';
}

function makeHistoricChart()
{
    var unusable = Object.keys(removeYears).length;
    var subtitle = historicGraphData['Count'] + ' of ' + historicGraphData['Total'] +
                ' Years Selected for Forecast after End of Ensemble' +
                (unusable == 0 ?  "" : '<br />' + unusable + ' year' + (unusable == 1 ? ' was' : 's were') + ' unusable due to length of period requested extending into current year') +
                '<br />Overall Match Quality: ' +  rateGrade(historicGraphData['OverallGrade']);
    var data = [];
    for (var ri in Object.keys(rateColour))
    {
        var r = Object.keys(rateColour)[ri];
        data.push({
                    backgroundColor: rateColour[r],
                    label: r,
                    data: cjs_historicDatasets[r],
                    categoryPercentage: 1,
                    barPercentage: .6,
                    borderColor: '#7cb5ec',
                    borderWidth: 1,
        });
    }
    var barChartData = {
        labels: cjs_historicLabels,
        datasets: data
    };
    var ctx = document.getElementById('historic').getContext('2d');
    if (!window.charts)
    {
        window.charts = {};
    }
    window.charts['historic'] = new Chart(ctx, {
        type: 'bar',
        data: barChartData,
        options: {
            responsive: true,
            legend: {
                position: 'bottom',
            },
            title: {
                display: true,
                text: ['Historic Year Match Quality'].concat(subtitle.split('<br />')),
            },
            scales : {
                xAxes: [{
                    gridLines: {
                        display: false,
                    },
                    ticks: {
                        autoSkip: false,
                        maxRotation: 90,
                        minRotation: 90,
                    },
                    stacked: true,
                    scaleLabel: {
                        display: true,
                        labelString: 'Year',
                    }
                }],
                yAxes: [{
                    stacked: true,
                    gridLines: {
                        display: false,
                    },
                    scaleLabel: {
                        display: true,
                        labelString: 'Grade (log)',
                    },
                    display: true,
                    type: 'logarithmic',
                    ticks: {
                        autoSkip: true,
                        min: 0.1,
                        max: 10,
                        callback: function (value, index, values) {
                            if( value == 0.1 || value == 1 || value == 10){
                                return value;
                            }
                        }
                    },
                }]
            },
            tooltips: {
                callbacks: {
                    label: function(tooltipItem, data) {
                        var label = 'Ranked #' + (cjs_historicLabels.indexOf(tooltipItem.xLabel) + 1);
                        if (label) {
                            label += ': ';
                        }
                        label += '(Grade ' + Math.round(tooltipItem.yLabel * 10) / 10 + ' / 10)';
                        return label;
                    }
                }
            },
            annotation: {
                drawTime: 'beforeDatasetsDraw',
                annotations: [{
                        type: 'line',
                        mode: 'vertical',
                        value: cjs_historicLabels[(Math.round(historicGraphData['Count'] - 1) / 2)],
                        lineWidth: 0,
                        scaleID: 'x-axis-0',
                        borderColor: 'rgba(0, 0, 0, 0)',
                        borderWidth: 0,
                        label: {
                            backgroundColor: 'rgba(0, 0, 0, 0)',
                            fontColor: 'rgba(0, 0, 0, 1)',
                            content: 'Selected',
                            enabled: true,
                            yAdjust: -$('#historic').height() / 4,
                        },
                    },{
                        type: 'line',
                        mode: 'vertical',
                        value: cjs_historicLabels[Math.round((historicGraphData['Total'] - historicGraphData['Count']) / 2) + historicGraphData['Count']- 1],
                        lineWidth: 0,
                        scaleID: 'x-axis-0',
                        borderColor: 'rgba(0, 0, 0, 0)',
                        borderWidth: 0,
                        label: {
                            backgroundColor: 'rgba(0, 0, 0, 0)',
                            fontColor: 'rgba(0, 0, 0, 1)',
                            content: 'Excluded',
                            enabled: true,
                            yAdjust: -$('#historic').height() / 4,
                        },
                    },{
                        type: 'box',
                        xMax: cjs_maxSelected,
                        xScaleID: 'x-axis-0',
                        yScaleID: 'y-axis-0',
                        yMin: 0,
                        yMax: 10,
                        backgroundColor: 'rgba(0, 255, 0, 0.2)',
                        borderColor: 'rgb(101, 33, 171)',
                        borderWidth: 1,
                    },
                    {
                        type: 'box',
                        xMin: cjs_maxSelected,
                        xScaleID: 'x-axis-0',
                        yScaleID: 'y-axis-0',
                        yMin: 0,
                        yMax: 10,
                        backgroundColor: 'rgba(0, 0, 0, 0.1)',
                        borderColor: 'rgb(101, 33, 171)',
                        borderWidth: 1,
                }]
            }
        }
    });
}


/*
 * Output the source information to the bottom of the page
 * @param {type} sourceDates
 * @returns {undefined}
 */
function setSourceInfo(multiArray){
    var sourceDates = getSourceDetails(multiArray);
    var startup =   multiArray['StartupValues'];
    var haveAFFES = false;
    var affesDays = 0;
    var affesRows = [];
    var indicesNames = {
        'TMP': 'Temp',
        'WD': 'Dir',
        'WS': 'Speed',
        'APCP': 'Rain'
    };
    var indicesOrder = ['TMP', 'RH', 'WD', 'WS', 'APCP', 'FFMC', 'DMC', 'DC', 'ISI', 'BUI', 'FWI'];
    for (var v in indicesOrder)
    {
        var forVar = indicesOrder[v];
        var isShown = false;
        for (var c in showIndices)
        {
            isShown = isShown || (showIndices[c] == forVar);
        }
        if (isShown)
        {
            var name = (forVar in indicesNames) ? indicesNames[forVar] : forVar;
            var index = getAFFESForecast(multiArray, forVar);
            if (null != index)
            {
                if (!haveAFFES)
                {
                    haveAFFES = true;
                    affesRows[0] = "<table style='color:black'><caption>AFFES Forecast (for nearest point)</caption><tr><th>Date</th>";
                    var i = 0;
                    affesDays = Math.min(5, index.length - 1);
                    while (i <= affesDays && null != index[i])
                    {
                        i++;
                    }
                    affesDays = i;
                    var dateArray = multiArray['ForDates'].map(function (x)
                                                            {
                                                                return shortDate(x, 28 >= multiArray['ForDates'].length);
                                                            });
                    for (var j = 0; j < i; j++)
                    {
                        affesRows[j + 1] = "<tr><td>" + dateArray[j] + "</td>";
                    }
                }
                affesRows[0] += "<th>" + name + "</th>";
                for (var j = 0; j <= affesDays; j++)
                {
                    affesRows[j + 1] += "<td>" + index[j] + "</td>";
                }
            }
        }
    }
    if (haveAFFES)
    {
        document.getElementById("affesHeader").innerHTML = "";
        var affesInfo = "";
        for (var j = 0; j <= affesDays; j++)
        {
            affesInfo += affesRows[j] + "</tr>";
        }
        affesInfo += "</table>";
        document.getElementById("affesInfo").innerHTML = affesInfo;
    }
    else
    {
        document.getElementById("affesHeader").innerHTML = "No AFFES Forecast available<br />";
    }
    var forecastInfo = (overrideFWI ? "FWI startup values manually entered" : "") + "<br>";
    forecastInfo += "<table style='color:black'><caption>CFFWI Station used</caption>" +
                    "<tr>" + 
                        "<th>Station</th>" +
                        "<th>Location</th>" +
                        "<th>Distance From</th>" +
                        "<th>Observed Date</th>" + 
                        "<th>FFMC</th>" +
                        "<th>DMC</th>" +
                        "<th>DC</th>" +
                        "<th>0800 Precip.</th>" +
                    "</tr><tr>";
    if (null != startup.Station && !overrideFWI) {
        forecastInfo +=  "<td>" + startup.Station + "</td>" +
                            "<td>" + startup.lat + ' x ' + startup.lon + "</td>" +
                            "<td>" + (startup.DistanceFrom / 1000).toFixed(2) + "km</td>" +
                            "<td>" + new Date(startup.Generated).toUTCString() +"</td>";
    }
    else
    {
        forecastInfo +=  "<td>N/A</td>" +
                        "<td>N/A</td>" +
                        "<td>N/A</td>" +
                        "<td>N/A</td>";
    }
    forecastInfo += "<td>" + startup.FFMC.toFixed(1) + "</td>" +
                        "<td>" + startup.DMC.toFixed(0) + "</td>" +
                        "<td>" + startup.DC.toFixed(0) + "</td>" +
                        "<td>" + startup.APCP_0800 + "mm</td>" +
                    "</tr></table>";
    forecastInfo += "<table style='color:black' id='tblMembers' name='tblMembers'><caption>Model Data:</caption><tr><th>Source</th><th>Members</th><th>Closest Point</th><th>Distance From</th><th>Forecast Date</th><th>Notes</th></tr>";
   
    for(var i=0;i<sourceDates.Source.length;i++){
        forecastInfo += "<tr><td>" + sourceDates.Source[i] +
                        "</td><td>" + sourceDates.Members[i] +
                        "</td><td>" + sourceDates.Latitude[i] + ' x ' + sourceDates.Longitude[i] +
                        "</td><td>" + (sourceDates.DistanceFrom[i] / 1000).toFixed(2) + 'km' +
                        "</td><td>" + sourceDates.theDate[i].toUTCString() +
                        "</td><td>" + sourceDates.Notes[i] + "</td></tr>";
    }
    
    var count = 0;
    var sum_grade = 0.0;
    var included_line = 0;
    var matches = multiArray['Matches'];
    if (matches) {
        for (var r in matches['Order']) {
                var year = matches['Order'][r];
                var grade = matches['Grades'][year];
                // NOTE: compare grade before adding value so that we pick last value that started before MAX_GRADE
                if (sum_grade < MAX_GRADE) {
                     if (!(year in removeYears))
                    {
                        // HACK: use abs of grade so that negative years still increase sum
                        sum_grade += Math.abs(grade);
                        count++;
                    }
                    included_line++;
                    cjs_maxSelected = year;
                }
                historicGraphData['Total']++;
                historicCategories.push(year);
                for (var rate in rateColour)
                {
                    var v = Math.max(10 * grade, 0.0000001);
                    if (year in removeYears)
                    {
                        if (rate != 'Unusable')
                        {
                            //~ v = (rate == 'Unusable' ? (10 * grade) : 0.0);
                            //~ v = 0.0;
                            historicBarData[rate].push(null);
                        }
                        else
                        {
                            historicBarData[rate].push(
                                {
                                    name: year,
                                    // HACK: avoid error because of log axis
                                    y: v,
                                    grade: 10 * grade,
                                    color: rateColour[rate],
                                    rank: historicGraphData['Total']
                                }
                            );
                        }
                    }
                    else if (rateGrade(grade) != rate)
                    {
                        //~ v = 0.0;
                        historicBarData[rate].push(null);
                    }
                    else
                    {
                    cjs_historicLabels.push(year)
                    cjs_historicColors.push(rateColour[rate]);
                    var label = year +'\nRanked #' + (cjs_historicBarLabels.length + 1);
                    cjs_historicBarLabels.push(label);
                    for (var ri in Object.keys(cjs_historicDatasets))
                    {
                        var r = Object.keys(cjs_historicDatasets)[ri];
                        if (r != rate)
                        {
                            cjs_historicDatasets[r].push(0);
                        }
                    }
                            cjs_historicDatasets[rate].push(v);
                        historicBarData[rate].push(
                            {
                                name: year,
                                // HACK: avoid error because of log axis
                                y: v,
                                grade: 10 * grade,
                                color: rateColour[rate],
                                rank: historicGraphData['Total']
                            }
                        );
                    }
                }
        }
        var m = Object.keys(multiArray['Hindcast'])[0];
        var model = multiArray['Hindcast'][m];
        forecastInfo += "<tr><td>" + m +
                        "</td><td>" + count +
                        "</td><td>" + model['lat'].toFixed(4) + ' x ' + model['lon'].toFixed(4) +
                        "</td><td>" + (model.DistanceFrom / 1000).toFixed(2) + 'km' +
                        "</td><td>" + new Date(model.Generated).toUTCString() + 
                        "</td><td></td></tr>";
    }
    forecastInfo += "</table>";
    historicGraphData['OverallGrade'] = sum_grade / count;
    historicGraphData['Count'] = count;
    historicGraphData['IncludedLine'] = included_line;
    if (0 < count)
    {
        $('#spanHistoricBar').css('display', 'block');
    }
    forecastInfo += "</table>";
    
    document.getElementById("forecastInfo").innerHTML = forecastInfo;
}

function resizeToDataUrl(canvas, width, height)
{
    var DPI = 300;
    var resizedCanvas = document.createElement("canvas");
    var ctx = resizedCanvas.getContext("2d");
    resizedCanvas.width = DPI * width;
    resizedCanvas.height = DPI * height;
    ctx.imageSmoothingQuality = 'high';
    ctx.drawImage(canvas, 0, 0, resizedCanvas.width, resizedCanvas.height);
    var imgData = ctx.getImageData(0, 0, resizedCanvas.width, resizedCanvas.height);
    var data = imgData.data;
    for(var i=0; i < data.length; i += 4)
    {
        // convert every transparent colour to be an opaque colour with a white background
        var scale = data[i + 3] / 255.0;
        var white = (1 - scale) * 255;
        data[i] = Math.round(data[i] * scale + white);
        data[i + 1] = Math.round(data[i + 1] * scale + white)
        data[i + 2] = Math.round(data[i + 2] * scale + white)
        data[i + 3] = 255;
    }
    ctx.putImageData(imgData, 0, 0);
    return resizedCanvas.toDataURL('image/jpeg', 0.8);
}


function doExport11x17()
{
    // All units are in the set measurement for the document
    // This can be changed to "pt" (points), "mm" (Default), "cm", "in"
    var PAGE_WIDTH = 17;
    var PAGE_HEIGHT = 11;
    var doc = new jsPDF('l', 'in', [PAGE_WIDTH, PAGE_HEIGHT]);
    
    function getLineHeight()
    {
        //~ return (6.0 + doc.internal.getFontSize()) / 792.0 * 9.0;
        return doc.getLineHeight() / 72.0;
    }
    
    //Set where the page starts for images, this will change after page 1
    var pageHeight = 0;
    var chartWidth = 5.5;
    var marginX = 0.25;
    var marginY = 0.25;
    var curY = marginY;
    var AFTER_TITLE_Y;
    var column = 0;
    var NUM_COLUMNS = 3;
    var COLUMN_WIDTH = (PAGE_WIDTH - 2.0 * marginX) / NUM_COLUMNS;
    function getFontSize(id)
    {
        return parseInt($('#' + id).css('font-size')) / 2;
    }
    var titleFontSize = getFontSize('title');
    var fontRatio = titleFontSize / 16.0;
    function getScaledFontSize(id)
    {
        return fontRatio * getFontSize(id);
    }
    var pageNumberFontSize = getScaledFontSize('disclaimer');
    //~ doc.setFontSize(pageNumberFontSize);
    var pageNumberX = PAGE_WIDTH - (doc.getTextWidth('Page MMM') + marginX);
    var pageNumberY = PAGE_HEIGHT - marginY;
    //Find out if we are at a new page or not
    function addText(text, left) {
        // need to add height first because text position seems to be based on bottom of text
        curY += getLineHeight();
        checkNewPage(curY);
        if (!left) {
            // HACK: IE doesn't support center so can't use:
            // doc.text(text, PAGE_WIDTH / 2, curY, null, null, 'center');
            left = marginX + COLUMN_WIDTH * column + (COLUMN_WIDTH - doc.getTextWidth(text)) / 2;
        }
        doc.text(left, curY, text);
    }
    function addSection(id, left) {
        if (!$('#' + id).is(":visible"))
        {
            return;
        }
        var nodes = $('#' + id).contents();
        for (var i = 0; i < nodes.length; i++) {
            var line = '';
            // HACK: do this so that <a> and other things are included in output
            while (i < nodes.length && (nodes[i].nodeName != 'BR'))
            {
                line += nodes[i].textContent;
                i++;
            }
            if ('' != line.trim()) {
                addText(line.trim(), left);
            }
        }
    }
    function addImage(img, left, top, height)
    {
        var canvas = document.createElement('canvas');
        var context = canvas.getContext('2d');
        var img = document.getElementById(img);
        canvas.width = img.naturalWidth;
        canvas.height = img.naturalHeight;
        var ratio = canvas.width / (1.0 * canvas.height);
        context.drawImage(img, 0, 0);
        var scale = (1.0 * height) / canvas.height;
        context.scale(scale, scale);
        var myData = canvas.toDataURL('image/png');
        var scaledWidth = img.naturalWidth * scale;
        var scaledHeight = img.naturalHeight * scale;
        var centeredLeft = marginX + COLUMN_WIDTH * column + left - (COLUMN_WIDTH - scaledWidth) / 2;
        doc.addImage(myData, 'png', centeredLeft, top, scaledWidth, scaledHeight);
    }
    var noNewline = false;
    function addChart(index)
    {
        if (!$(this).is(":visible"))
        {
            return;
        }
        var chart = window.charts[$(this).attr('id')];
        chart.width = chart.canvas.scrollWidth;
        chart.height = chart.canvas.scrollHeight;
        var ratioWidth = parseInt($(this).parent().css('max-width')) / 1050.0;
        // add 2 due to padding
        var chartRatio = (this.clientHeight + 2) / (1.0 * (this.clientWidth + 2));
        var width = chartWidth * ratioWidth;
        var chartHeight = width * chartRatio;
        checkNewPage(chartHeight + curY);
        var left = marginX + COLUMN_WIDTH * column + (COLUMN_WIDTH - width) / 2;
        var url_base64jp = resizeToDataUrl(chart.canvas, width, chartHeight);
        doc.addImage(url_base64jp, 'jpeg', left, curY, width, chartHeight);
        if (!noNewline)
        {
            curY += chartHeight;
        }
    }
    function checkNewPage(pageHeight)
    {
        // if we would go past page number location
        if (pageHeight >= pageNumberY){
            if (NUM_COLUMNS == column)
            {
                doc.page++;
                doc.addPage();
            }
            else
            {
                column += 1;
            }
            //~ curY = (1 == column) ? AFTER_TITLE_Y : marginY;
        curY = AFTER_TITLE_Y;
        }
    }
    function addTable(table)
    {
        var rows = table.childNodes[1].childNodes;
        checkNewPage(curY + getLineHeight() * rows.length);
        // add an empty line before the table
        var initialY = curY + getLineHeight();
        // HACK: push margin over even more because they seem too far left
        var left = 2 * marginX + COLUMN_WIDTH * column;
        // HACK: assume all rows have same number of columns
        for (var c = 0; c < rows[0].childNodes.length; c++) {
            var maxWidth = 0;
            curY = initialY;
            for (var i = 0; i < rows.length; i++)
            {
                var cell = rows[i].childNodes[c];
                //write separate containers so alignment is correct
                doc.text(left, curY, cell.textContent);
                curY += getLineHeight();
                maxWidth = Math.max(maxWidth, doc.getTextWidth(cell.textContent));
            }
            // add max text width for column and then the width on an 'm' for padding between cells
            left += maxWidth + doc.getTextWidth('m');
        }
    }
    // begin generation
    doc.page = 1;
    column = 1;
    doc.setFontSize(getScaledFontSize("title"));
    addSection("title");
    doc.setFontSize(getScaledFontSize("reportTitle"));
    addSection('reportTitle');
    // size these according to how big the title section is
    var imageHeight = (curY - marginY) * 1.2;
    addImage('imgLogo', 2, marginY * 0.6, imageHeight);
    addImage('imgAFFES', 6.5, marginY * 0.6, imageHeight);
    curY = (marginY + imageHeight / 1.2) * 1.1;
    addSection('noIndices', marginX);
    AFTER_TITLE_Y = curY;
    column = 0;
    curY = AFTER_TITLE_Y;
    //loop through each chart
    $('#TMP').each(addChart);
    $('#RH').each(addChart);
    $('#FFMC').each(addChart);
    $('#ISI').each(addChart);
    doc.setFontSize(8);
    addSection('affesHeader', marginX * 2)
    var affesTables = $('#affesInfo').children().filter('table');
    affesTables.each(function (i) { addTable(affesTables[i]); });
    column = 1;
    curY = AFTER_TITLE_Y;
    $('#WS').each(addChart);
    column = 0.625;
    noNewline = true;
    $('#WDDay1').each(addChart);
    column += 0.25;
    $('#WDDay2').each(addChart);
    column += 0.25;
    $('#WDDay3').each(addChart);
    noNewline = false;
    column += 0.25;
    $('#WD').each(addChart);
    column = 1;
    $('#DMC').each(addChart);
    $('#BUI').each(addChart);
    doc.setFontSize(8);
    addSection('disclaimer', 2 * marginX + column * COLUMN_WIDTH);
    $('#APCP').each(addChart);
    $('#Accum').each(addChart);
    $('#DC').each(addChart);
    $('#FWI').each(addChart);
    doc.setFontSize(7);
    var tables = $('#forecastInfo').children().filter('table');
    tables.each(function (i) { addTable(tables[i]); });
    column = 1;
    curY = 0;
    addText(ACTIVE_OFFER, 0);
    //~ curY += getLineHeight();
    //save with name
    var filename = ((null != theFire) ? (theFire + "_") : "") + strStartDate + "_wxshield.pdf";
    doc.save(filename);
}




function doExport()
{
    var PAGE_WIDTH = 8.5;
    var PAGE_HEIGHT = 11;
    // All units are in the set measurement for the document
    // This can be changed to "pt" (points), "mm" (Default), "cm", "in"
    var doc = new jsPDF('p', 'in', [PAGE_WIDTH, PAGE_HEIGHT]);
    
    function getLineHeight()
    {
        //~ return (6.0 + doc.internal.getFontSize()) / 792.0 * 9.0;
        return doc.getLineHeight() / 72.0;
    }
    
    //Set where the page starts for images, this will change after page 1
    var pageHeight = 0;
    var chartWidth = 8;
    var marginX = 0.25;
    var marginY = 0.25;
    var curY = marginY;
    var column = 0;
    var COLUMN_WIDTH = PAGE_WIDTH - marginX * 2;
    var TARGET_WIDTH = 1050.0 / 0.85;
    function getFontSize(id)
    {
        return parseInt($('#' + id).css('font-size')) / 2;
    }
    var titleFontSize = getFontSize('title');
    var fontRatio = titleFontSize / 16.0;
    function getScaledFontSize(id)
    {
        return fontRatio * getFontSize(id);
    }
    var pageNumberFontSize = getScaledFontSize('disclaimer');
    doc.setFontSize(pageNumberFontSize);
    var pageNumberX = PAGE_WIDTH - (doc.getTextWidth('Page MMM') + marginX);
    var pageNumberY = PAGE_HEIGHT - marginY;
    //Find out if we are at a new page or not
    function addText(text, left) {
        // need to add height first because text position seems to be based on bottom of text
        curY += getLineHeight();
        checkNewPage(curY);
        if (!left) {
            // HACK: IE doesn't support center so can't use:
            // doc.text(text, PAGE_WIDTH / 2, curY, null, null, 'center');
            left = (PAGE_WIDTH - doc.getTextWidth(text)) / 2.0;
        }
        doc.text(left, curY, text);
    }
    function addSection(id, left) {
        if (!$('#' + id).is(":visible"))
        {
            return;
        }
        doc.setFontSize(getScaledFontSize(id));
        var nodes = $('#' + id).contents();
        for (var i = 0; i < nodes.length; i++) {
            var line = '';
            // HACK: do this so that <a> and other things are included in output
            while (i < nodes.length && (nodes[i].nodeName != 'BR'))
            {
                line += nodes[i].textContent;
                i++;
            }
            if ('' != line.trim()) {
                addText(line.trim(), left);
            }
        }
    }
    function addImage(img, left, top, height)
    {
        var canvas = document.createElement('canvas');
        var context = canvas.getContext('2d');
        var img = document.getElementById(img);
        canvas.width = img.naturalWidth;
        canvas.height = img.naturalHeight;
        var ratio = canvas.width / (1.0 * canvas.height);
        context.drawImage(img, 0, 0);
        var scale = (1.0 * height) / canvas.height;
        context.scale(scale, scale);
        var myData = canvas.toDataURL('image/png');
        var scaledWidth = img.naturalWidth * scale;
        var scaledHeight = img.naturalHeight * scale;
        var centeredLeft = left - scaledWidth / 2.0;
        doc.addImage(myData, 'png', centeredLeft, top, scaledWidth, scaledHeight);
    }
    var noNewline = false;
    function addChart(index)
    {
        if (!$(this).is(":visible"))
        {
            return;
        }
        var chart = window.charts[$(this).attr('id')];
        chart.width = chart.canvas.scrollWidth;
        chart.height = chart.canvas.scrollHeight;
        var ratioWidth = parseInt($(this).parent().css('max-width')) / TARGET_WIDTH;
        // add 2 due to padding
        var chartRatio = (this.clientHeight + 2) / (1.0 * (this.clientWidth + 2));
        var width = chartWidth * ratioWidth;
        var chartHeight = width * chartRatio;
        checkNewPage(chartHeight + curY);
        var left = marginX + COLUMN_WIDTH * column + (COLUMN_WIDTH - width) / 2;
        var url_base64jp = resizeToDataUrl(chart.canvas, width, chartHeight);
        doc.addImage(url_base64jp, 'jpeg', left, curY, width, chartHeight);
        if (!noNewline)
        {
            curY += chartHeight;
        }
    }
    function numberPage()
    {
        doc.setFontSize(pageNumberFontSize);
        doc.text(pageNumberX, pageNumberY, 'Page ' + doc.page);
    }
    function checkNewPage(pageHeight)
    {
        // if we would go past page number location
        if (pageHeight >= pageNumberY){
            numberPage();
            doc.page++;
            doc.addPage();
            
            curY = marginY;
        }
    }
    function addTable(table)
    {
        var rows = table.childNodes[1].childNodes;
        checkNewPage(curY + getLineHeight() * rows.length);
        // add an empty line before the table
        var initialY = curY + getLineHeight();
        var left = marginX;
        // HACK: assume all rows have same number of columns
        for (var c = 0; c < rows[0].childNodes.length; c++) {
            var maxWidth = 0;
            curY = initialY;
            for (var i = 0; i < rows.length; i++)
            {
                var cell = rows[i].childNodes[c];
                //write separate containers so alignment is correct
                doc.text(left, curY, cell.textContent);
                curY += getLineHeight();
                maxWidth = Math.max(maxWidth, doc.getTextWidth(cell.textContent));
            }
            // add max text width for column and then the width on an 'm' for padding between cells
            left += maxWidth + doc.getTextWidth('m');
        }
    }
    // begin generation
    doc.page = 1;
    addSection("title");
    addSection('reportTitle');
    oldY = curY;
    curY = 0;
    addText(ACTIVE_OFFER, 0);
    curY = oldY;
    // size these according to how big the title section is
    var imageHeight = (curY - marginY);
    addImage('imgLogo', 2, marginY * 1.1, imageHeight);
    addImage('imgAFFES', 6.5, marginY * 1.1, imageHeight);
    curY = (marginY + imageHeight) * 1.1;
    addSection('noIndices', marginX);
    //loop through each chart
    $('.chart1 > canvas').each(addChart);
    $('.chart2 > canvas').each(addChart);
    column = -0.375;
    noNewline = true;
    $('#WDDay1').each(addChart);
    column += 0.25;
    $('#WDDay2').each(addChart);
    column += 0.25;
    $('#WDDay3').each(addChart);
    noNewline = false;
    column += 0.25;
    $('#WD').each(addChart);
    column = 0;
    $('.chart4 > canvas').each(addChart);
    $('.chart5 > canvas').each(addChart);
    $('.chart6 > canvas').each(addChart);
    $('.chart7 > canvas').each(addChart);
    curY += getLineHeight();
    addSection('affesHeader', marginX)
    var affesTables = $('#affesInfo').children().filter('table');
    affesTables.each(function (i) { addTable(affesTables[i]); });
    addSection('disclaimer', marginX);
    var tables = $('#forecastInfo').children().filter('table');
    tables.each(function (i) { addTable(tables[i]); });
    numberPage();
    //save with name
    var filename = ((null != theFire) ? (theFire + "_") : "") + strStartDate + "_wxshield.pdf";
    doc.save(filename);
}


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
    // var dataUrl = "https://app-cwfmf-api-cwfis-dev.wittyplant-59b495b3.canadacentral.azurecontainerapps.io/gribwx?lat=" + theLat + "&lon=" + theLong + "&model=all&format=csv";
    var MODEL = "hrdps";
    // var MODEL = "all";
    var dataUrl = "https://app-cwfmf-api-cwfis-dev.wittyplant-59b495b3.canadacentral.azurecontainerapps.io/gribwx?lat=" + theLat + "&lon=" + theLong + "&model=" + MODEL + "&format=csv";
    if (isDebug) {
        $('#datalink').prop('href', dataUrl);
    }
    xmlhttp.open("GET", dataUrl, true);
    xmlhttp.send();
    // xmlhttp.onreadystatechange = function () { console.log("done"); };
    xmlhttp.onreadystatechange = function () {
        if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
            var multiArray = [];
            // try {
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
                    // console.log(i);
                    assert(i < lines.length);
                    // if (0 != i && i < lines.length) {
                    line = lines[i];
                    cols = line.split(",");
                    if (0 == i) {
                        indices = cols.slice(2);
                    } else if (0 < line.length) {
                        j = 0;
                        if (model != "all")
                        {
                            model = MODEL;
                        }
                        else
                        {
                            model = cols[j];
                            ++j;
                            member = 0;
                            g = model.match(/^([^\d]+)([\d]+)$/);
                            if (g != null) {
                                model = g[1];
                                member = parseInt(g[2]);
                            }
                        }
                        for_model = models[model] || {};
                        for_members = for_model["Members"] || {};
                        for_member = for_members[member] || {};
                        // FIX: worry about UTC later - it will use sytem timezone for now
                        datetime = new Date(Date.parse(cols[j]));
                        ++j;
                        date = new Date(datetime.getFullYear(), datetime.getMonth(), datetime.getDay());
                        for_date = for_member[date] || {};
                        // assign actual indices here
                        for_date[datetime.getHours()] = cols.slice(j).map(parseFloat);
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
                all_dates = new Set();
                for (var model in models) {
                    for_model = models[model]["Members"];
                    for (var member in for_model) {
                        for_member = for_model[member];
                        for (var date in for_member) {
                            all_dates.add(date);
                            for_date = for_member[date];
                            min_time = Infinity;
                            apcp = 0;
                            for (var hour in for_date) {
                                for_hour = for_date[hour];
                                // console.log([model, member, date, for_date, hour, for_hour]);
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
                        // HACK: remove dates from members and use index instead
                        // HACK: assume sorted and in same index as all_dates will be
                        for_model[member] = Array.from(Object.values(for_member));
                    }
                    // HACK: members need to be 0 based
                    models[model]["Members"] = Array.from(Object.values(for_model));
                }
                multiArray['Models'] = models;
                // HACK: assume sorted for now
                multiArray['ForDates'] = Array.from(all_dates);
                // convert date strings into actual dates right away
                multiArray['ForDates'] = multiArray['ForDates'].map(function (currentValue, index, array) {
                    return new Date(currentValue);
                });
                multiArray["StartDate"] = multiArray['ForDates'][0];
                for (var model in multiArray['Models'])
                {
                    // HACK: use today for now
                    multiArray['Models'][model]['Generated'] = multiArray["StartDate"];
                }
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
            // }
            // catch (e) {
            //     document.getElementById("msg").innerHTML = "Error:" + e + " " + xmlhttp.responseText;
            // }
        }
    }
}


function fixFront(e)
{
    if (e)
    {
        e.toFront();
    }
}

function loadAll()
{
    $('#export_letter').click(doExport);
    $('#export_tabloid').click(doExport11x17);
    
    
    document.getElementById("msg").innerHTML = "Please wait while data loads.";
    loadData();
}

function exportCSV(showIndices, inputData, filename)
{
    var colDelim = ',';
    var rowDelim = '\r\n';

    var csv = '';
    csv += [
        'Scenario',
        'Date'
        ].join(colDelim);
    // HACK: remove Accum column
    csv += colDelim + showIndices.join(colDelim).replace(',Accum', '') + rowDelim;
    var arrayOfEnsembles = inputData['Models'];
    var dates = inputData['ForDates'];
    var apcp_adj = inputData['StartupValues']['APCP_0800'];
    if ('Observed' in inputData['Actuals'])
    {
        var obsStation = Object.keys(inputData['Actuals']['Observed']['Members'])[0];
        var observed = inputData['Actuals']['Observed']['Members'][obsStation];
        if (null != observed)
        {
            var indices = inputData['Indices'];
            // need to pick the i'th value of every day from this array
            for (var d = 0; d < observed.length; d++)
            {
                csv += [
                    "actual",
                    formatDate(dates[d]) + ' 13:00'
                ].join(colDelim);
                for (var k in showIndices)
                {
                    var index = indices.indexOf(showIndices[k]);
                    if ("Accum" != showIndices[k])
                    {
                        // don't output Accum since it can easily be calculated if needed
                        csv += colDelim + Number(observed[d][index].toFixed(1));
                    }
                }
                csv += rowDelim;
            }
        }
    }
    
    var wx_data = CALCULATED_WEATHER['Models'];
    var numberStreams = wx_data[showIndices[0]][0].length;
    for (var i = 0; i < numberStreams; i++)
    {
        // need to pick the i'th value of every day from this array
        for (var d = 0; d < wx_data[showIndices[0]].length; d++)
        {
            csv += [
                i + 1,
                formatDate(dates[d]) + ' 13:00'
            ].join(colDelim);
            for (var k in showIndices)
            {
                var index = showIndices[k];
                if ("Accum" != showIndices[k])
                {
                    // don't output Accum since it can easily be calculated if needed
                    csv += colDelim + Number(wx_data[index][d][i].toFixed(1));
                }
            }
            csv += rowDelim;
        }
    }
    var blob = new Blob([csv], {type: 'text/csv'});
    if (window.navigator.msSaveBlob) { // IE 10+
        window.navigator.msSaveOrOpenBlob(blob, filename)
    } 
    else {
        var elem = window.document.createElement('a');
        elem.href = window.URL.createObjectURL(blob);
        elem.download = filename;
        document.body.appendChild(elem);
        elem.click();
        document.body.removeChild(elem);
    }
}
