library("cffdrs")
library("data.table")
library("jsonlite")
library("testthat")

# precision to ensure to consider things a match
TOLERANCE <- 0.001

DIR <- normalizePath("example")
FILE_JSON <- sprintf("%s/fire.geojson", DIR)
FILE_WX_OUTPUT <- sprintf("%s/output/wx_hourly_out.csv", DIR)

fire <- fromJSON(FILE_JSON)$features$properties
# fire <- unlist(fromJSON(FILE_JSON)$features$properties)
cols <- c("lat", "lon", "ffmc_old", "dmc_old", "dc_old")
init <- as.data.table(lapply(fire[cols], as.double),
    names = cols
)


read_wx <- function(path) {
    df <- fread(path)
    setnames(df, c("Scenario"), c("ID"))
    names(df) <- toupper(names(df))
    df[, `:=`(
        YR = year(DATE),
        MON = month(DATE),
        DAY = mday(DATE),
        HR = hour(DATE),
        MIN = minute(DATE)
    )]
    df[, `:=`(
        LAT = init$lat,
        LONG = init$lon
    )]
    setorder(df, "DATE", "ID")
    return(df)
}


# # don't worry about input csv hourly calculations for now
# df_fwi <- NULL
# for (id in unique(df_wx$ID)) {
#     df_fwi <- rbind(
#         df_fwi,
#         fwi(df_wx[ID == id], init = init)
#     )
# }
# setorder(df_fwi, "ID", "DATE")
# cols_cmp <- names(df)
# df_cmp <- df_fwi[, ..cols_cmp]
# df_cmp - df

# fwi()
df_wx_out <- read_wx(FILE_WX_OUTPUT)
df_wx <- df_wx_out[, -c("FFMC", "DMC", "DC", "ISI", "BUI", "FWI")]

cols_derived <- c("ISI", "BUI", "FWI")
# compare calcuation of dependent variabls
df_fwi <- df_wx_out[, -..cols_derived]
df_fwi[, `:=`(
    ISI = cffdrs:::initial_spread_index(FFMC, WS),
    BUI = cffdrs:::buildup_index(DMC, DC)
)]
df_fwi[, FWI := cffdrs:::fire_weather_index(ISI, BUI)]

check_diff <- function(a) {
    cols <- names(a)
    b <- df_fwi[, ..cols]
    testthat::expect_equal(a, b, tolerance = TOLERANCE)
}
check_diff(df_wx_out)
check_diff(df_wx_out[, ..cols_derived])

df_wx_out[, FWI := cffdrs:::fire_weather_index(ISI, BUI)]
check_diff(df_wx_out[, ..cols_derived])

# df_fbp <- fbp(df_wx_out, output = "ALL")
# TODO: FIX
# Error in slope_values[["WSV"]] : subscript out of bounds
# when no FUELTYPE defined

df_wx_out[, `:=`(
    GS = 0,
    DJ = yday(DATE),
    ASPECT = 0,
    SLOPE = 0,
    FUELTYPE = "C2"
)]


df_fbp <- fbp(df_wx_out, output = "ALL")
