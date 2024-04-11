library("cffdrs")
back_rate_of_spread <- cffdrs:::back_rate_of_spread
buildup_effect <- cffdrs:::buildup_effect
buildup_index <- cffdrs:::buildup_index
crown_base_height <- cffdrs:::crown_base_height
crown_fraction_burned <- cffdrs:::crown_fraction_burned
crown_fuel_load <- cffdrs:::crown_fuel_load
fire_intensity <- cffdrs:::fire_intensity
fire_weather_index <- cffdrs:::fire_weather_index
flank_rate_of_spread <- cffdrs:::flank_rate_of_spread
length_to_breadth <- cffdrs:::length_to_breadth
length_to_breadth_at_time <- cffdrs:::length_to_breadth_at_time
rate_of_spread_at_time <- cffdrs:::rate_of_spread_at_time
slope_adjustment <- cffdrs:::slope_adjustment
surface_fire_rate_of_spread <- cffdrs:::surface_fire_rate_of_spread
surface_fuel_consumption <- cffdrs:::surface_fuel_consumption
total_fuel_consumption <- cffdrs:::total_fuel_consumption

initial_spread_index <- function (ffmc, ws, fbpMod = FALSE)
{
    fm <- 147.27723 * (101 - ffmc)/(59.5 + ffmc)
    fW <- ifelse(ws >= 40 & fbpMod == TRUE, 12 * (1 - exp(-0.0818 *
                                                              (ws - 28))), exp(0.05039 * ws))
    fF <- 91.9 * exp(-0.1386 * fm) * (1 + (fm^5.31)/49300000)
    isi <- 0.208 * fW * fF
    return(isi)
}
foliar_moisture_content <- function (LAT, LONG, ELV, DJ, D0)  {
    FMC <- rep(-1, length(LAT))
    LATN <- rep(0, length(LAT))
    LATN <- ifelse(D0 <= 0,
                    ifelse(ELV <= 0,
                           46 + 23.4 * exp(-0.036 * (150 - LONG)),
                           43 + 33.7 * exp(-0.0351 * (150 - LONG))),
                   LATN)
    D0 <- ifelse(D0 <= 0,
                  ifelse(ELV <= 0,
                         151 * (LAT/LATN),
                         142.1 * (LAT/LATN) + 0.0172 * ELV),
                  D0)
    D0 <- round(D0, 0)
    ND <- abs(DJ - D0)
    FMC <- ifelse(ND < 30,
                  85 + 0.0189 * ND^2,
                  ifelse(ND >= 30 & ND < 50,
                         32.9 + 3.17 * ND - 0.0288 * ND^2,
                         120))
    return(FMC)
}
critical_surface_intensity <- function (FMC, CBH)
{
    CSI <- 0.001 * (CBH^1.5) * (460 + 25.9 * FMC)^1.5
    return(CSI)
}
rate_of_spread_extended <- function (FUELTYPE, ISI, BUI, FMC, SFC, PC, PDF, CC, CBH) {
    NoBUI <- rep(-1, length(ISI))
    d <- c("C1", "C2", "C3", "C4", "C5", "C6", "C7", "D1", "M1",
           "M2", "M3", "M4", "S1", "S2", "S3", "O1A", "O1B")
    a <- c(90, 110, 110, 110, 30, 30, 45, 30, 0, 0, 120, 100,
           75, 40, 55, 190, 250)
    b <- c(0.0649, 0.0282, 0.0444, 0.0293, 0.0697, 0.08, 0.0305,
           0.0232, 0, 0, 0.0572, 0.0404, 0.0297, 0.0438, 0.0829,
           0.031, 0.035)
    c0 <- c(4.5, 1.5, 3, 1.5, 4, 3, 2, 1.6, 0, 0, 1.4, 1.48,
            1.3, 1.7, 3.2, 1.4, 1.7)
    names(a) <- names(b) <- names(c0) <- d
    RSI <- rep(-1, length(ISI))
    RSI <- ifelse(FUELTYPE %in% c("C1", "C2", "C3", "C4", "C5",
                                  "C7", "D1", "S1", "S2", "S3"), as.numeric(a[FUELTYPE] *
                                                                                (1 - exp(-b[FUELTYPE] * ISI))^c0[FUELTYPE]), RSI)
    RSI <- ifelse(FUELTYPE %in% c("M1"), (PC/100 * rate_of_spread(rep("C2",
                                                                      length(ISI)), ISI, NoBUI, FMC, SFC, PC, PDF, CC, CBH) +
                                              (100 - PC)/100 * rate_of_spread(rep("D1", length(ISI)),
                                                                              ISI, NoBUI, FMC, SFC, PC, PDF, CC, CBH)), RSI)
    RSI <- ifelse(FUELTYPE %in% c("M2"), (PC/100 * rate_of_spread(rep("C2",
                                                                      length(ISI)), ISI, NoBUI, FMC, SFC, PC, PDF, CC, CBH) +
                                              0.2 * (100 - PC)/100 * rate_of_spread(rep("D1", length(ISI)),
                                                                                    ISI, NoBUI, FMC, SFC, PC, PDF, CC, CBH)), RSI)
    RSI_m3 <- rep(-99, length(ISI))
    RSI_m3 <- ifelse(FUELTYPE %in% c("M3"), as.numeric(a[["M3"]] *
                                                           ((1 - exp(-b[["M3"]] * ISI))^c0[["M3"]])), RSI_m3)
    RSI <- ifelse(FUELTYPE %in% c("M3"), (PDF/100 * RSI_m3 +
                                              (1 - PDF/100) * rate_of_spread(rep("D1", length(ISI)),
                                                                             ISI, NoBUI, FMC, SFC, PC, PDF, CC, CBH)), RSI)
    RSI_m4 <- rep(-99, length(ISI))
    RSI_m4 <- ifelse(FUELTYPE %in% c("M4"), as.numeric(a[["M4"]] *
                                                           ((1 - exp(-b[["M4"]] * ISI))^c0[["M4"]])), RSI_m4)
    RSI <- ifelse(FUELTYPE %in% c("M4"), (PDF/100 * RSI_m4 +
                                              0.2 * (1 - PDF/100) * rate_of_spread(rep("D1", length(ISI)),
                                                                                   ISI, NoBUI, FMC, SFC, PC, PDF, CC, CBH)), RSI)
    CF <- rep(-99, length(ISI))
    CF <- ifelse(FUELTYPE %in% c("O1A", "O1B"), ifelse(CC < 58.8,
                                                       0.005 * (exp(0.061 * CC) - 1), 0.176 + 0.02 * (CC - 58.8)),
                 CF)
    RSI <- ifelse(FUELTYPE %in% c("O1A", "O1B"), a[FUELTYPE] *
                      ((1 - exp(-b[FUELTYPE] * ISI))^c0[FUELTYPE]) * CF, RSI)
    CSI <- critical_surface_intensity(FMC, CBH)
    RSO <- surface_fire_rate_of_spread(CSI, SFC)
    RSI <- ifelse(FUELTYPE %in% c("C6"), intermediate_surface_rate_of_spread_c6(ISI),
                  RSI)
    RSC <- ifelse(FUELTYPE %in% c("C6"), crown_rate_of_spread_c6(ISI,
                                                                 FMC), NA)
    RSS <- ifelse(FUELTYPE %in% c("C6"), surface_rate_of_spread_c6(RSI,
                                                                   BUI), buildup_effect(FUELTYPE, BUI) * RSI)
    CFB <- ifelse(FUELTYPE %in% c("C6"), crown_fraction_burned_c6(RSC,
                                                                  RSS, RSO), crown_fraction_burned(RSS, RSO))
    ROS <- ifelse(FUELTYPE %in% c("C6"), rate_of_spread_c6(RSC,
                                                           RSS, CFB), RSS)
    ROS <- ifelse(ROS <= 0, 1e-06, ROS)
    return(list(ROS = ROS, CFB = CFB, CSI = CSI, RSO = RSO))
}
#    static const auto Latitude <- 49.3911
#    static const auto Longitude <- -84.7395
#    static const topo:::StartPoint ForPoint(Latitude Longitude)
#    const auto start_date <- 123
#    const auto year <- model.year()
#    const auto month <- 6
#    const auto day <- 15
#    const auto hour <- 12
#    const auto minute <- 0
#  static const wx:::Temperature TEMP(20.0)
#  static const wx:::RelativeHumidity RH(30.0)
#  static const wx:::Precipitation PREC(0.0)
#    const wx:::Dc dc(275)
#    const wx:::Dmc dmc(35.5)
#    const wx:::Bui bui(54 dmc dc)
#    const wx:::Ffmc ffmc(90)
latitude <- 49.3911
longitude <- -84.7395
fueltype <- "C-2"
temp <- 20
rh <- 30
prec <- 0
ws <- 20
wd <- 180
dc <- 275
dmc <- 35.5
slope <- 0
aspect <- 0
ffmc <- 90
# isi <- 11.7
# bui <- 54
fwi <- 25.9
isi <- initial_spread_index(ffmc, ws)
bui <- buildup_index(dmc, dc)
fwi <- fire_weather_index(isi, bui)
dj <- 166
df <- as.data.table(list(
    latitude = latitude,
    longitude = longitude,
    fueltype = fueltype,
    temp = temp,
    rh = rh,
    prec = prec,
    ws = ws,
    wd = wd,
    dc = dc,
    dmc = dmc,
    slope = slope,
    aspect = aspect,
    bui = bui,
    ffmc = ffmc,
    isi = isi,
    fwi = fwi,
    dj = dj
))
names(df) <- toupper(names(df))
# fire_behaviour_prediction(df, output = "ALL")
input <- df
output <- "ALL"

output <- toupper(output)
if (is.null(input)) {
    input <- data.frame(FUELTYPE = "C2", ACCEL = 0, DJ = 180,
                        D0 = 0, ELV = 0, BUIEFF = 1, HR = 1, FFMC = 90, ISI = 0,
                        BUI = 60, WS = 10, WD = 0, GS = 0, ASPECT = 0, PC = 50,
                        PDF = 35, CC = 80, GFL = 0.35, CBH = 3, CFL = 1,
                        LAT = 55, LONG = -120, FMC = 0, THETA = 0)
    input[, "FUELTYPE"] <- as.character(input[, "FUELTYPE"])
}
names(input) <- toupper(names(input))
ID <- input$ID
FUELTYPE <- toupper(input$FUELTYPE)
FFMC <- input$FFMC
BUI <- input$BUI
WS <- input$WS
WD <- input$WD
FMC <- input$FMC
GS <- input$GS
LAT <- input$LAT
LONG <- input$LONG
ELV <- input$ELV
DJ <- input$DJ
D0 <- input$D0
SD <- input$SD
SH <- input$SH
HR <- input$HR
PC <- input$PC
PDF <- input$PDF
GFL <- input$GFL
CC <- input$CC
THETA <- input$THETA
ACCEL <- input$ACCEL
ASPECT <- input$ASPECT
BUIEFF <- input$BUIEFF
CBH <- input$CBH
CFL <- input$CFL
ISI <- input$ISI
n0 <- nrow(input)
if (!exists("FUELTYPE") | is.null(FUELTYPE)) {
    warning(paste0("FuelType is a required input,", " default FuelType = C2 is used in the calculation"))
    FUELTYPE <- rep("C2", n0)
}
if (!exists("FFMC") | is.null(FFMC)) {
    warning(paste0("FFMC is a required input, default FFMC = 90 is used in the",
                   " calculation"))
    FFMC <- rep(90, n0)
}
if (!exists("BUI") | is.null(BUI)) {
    warning("BUI is a required input, default BUI = 60 is used in the calculation")
    BUI <- rep(60, n0)
}
if (!exists("WS") | is.null(WS)) {
    warning("WS is a required input, WS = 10 km/hr is used in the calculation")
    WS <- rep(10, n0)
}
if (!exists("GS") | is.null(GS)) {
    warning("GS is a required input,GS = 0 is used in the calculation")
    GS <- rep(0, n0)
}
if (!exists("LAT") | is.null(LAT)) {
    warning("LAT is a required input, default LAT=55 is used in the calculation")
    LAT <- rep(55, n0)
}
if (!exists("LONG") | is.null(LONG)) {
    warning("LONG is a required input, LONG = -120 is used in the calculation")
    LONG <- rep(-120, n0)
}
if (!exists("DJ") | is.null(DJ)) {
    warning("Dj is a required input, Dj = 180 is used in the calculation")
    DJ <- rep(180, n0)
}
if (!exists("ASPECT") | is.null(ASPECT)) {
    warning("Aspect is a required input, Aspect = 0 is used in the calculation")
    ASPECT <- rep(0, n0)
}
if (!exists("WD") | is.null(WD)) {
    WD <- rep(0, n0)
}
if (!exists("FMC") | is.null(FMC)) {
    FMC <- rep(0, n0)
}
if (!exists("ELV") | is.null(ELV)) {
    ELV <- rep(0, n0)
}
if (!exists("SD") | is.null(SD)) {
    SD <- rep(0, n0)
}
if (!exists("SH") | is.null(SH)) {
    SH <- rep(0, n0)
}
if (!exists("D0") | is.null(D0)) {
    D0 <- rep(0, n0)
}
if (!exists("HR") | is.null(HR)) {
    HR <- rep(1, n0)
}
if (!exists("PC") | is.null(PC)) {
    PC <- rep(50, n0)
}
if (!exists("PDF") | is.null(PDF)) {
    PDF <- rep(35, n0)
}
if (!exists("GFL") | is.null(GFL)) {
    GFL <- rep(0.35, n0)
}
if (!exists("CC") | is.null(CC)) {
    CC <- rep(80, n0)
}
if (!exists("THETA") | is.null(THETA)) {
    THETA <- rep(0, n0)
}
if (!exists("ACCEL") | is.null(ACCEL)) {
    ACCEL <- rep(0, n0)
}
if (!exists("BUIEFF") | is.null(BUIEFF)) {
    BUIEFF <- rep(1, n0)
}
if (!exists("CBH") | is.null(CBH)) {
    CBH <- rep(0, n0)
}
if (!exists("CFL") | is.null(CFL)) {
    CFL <- rep(0, n0)
}
if (!exists("ISI") | is.null(ISI)) {
    ISI <- rep(0, n0)
}
WD <- WD * pi/180
THETA <- THETA * pi/180
ASPECT <- ifelse(is.na(ASPECT), 0, ASPECT)
ASPECT <- ifelse(ASPECT < 0, ASPECT + 360, ASPECT)
ASPECT <- ASPECT * pi/180
ACCEL <- ifelse(is.na(ACCEL) | ACCEL < 0, 0, ACCEL)
if (length(ACCEL[!ACCEL %in% c(0, 1)]) > 0) {
    warning("Input variable Accel is out of range, will be assigned to 1")
}
ACCEL <- ifelse(!ACCEL %in% c(0, 1), 1, ACCEL)
DJ <- ifelse(DJ < 0 | DJ > 366, 0, DJ)
DJ <- ifelse(is.na(DJ), 180, DJ)
D0 <- ifelse(is.na(D0) | D0 < 0 | D0 > 366, 0, D0)
ELV <- ifelse(ELV < 0 | ELV > 10000, 0, ELV)
ELV <- ifelse(is.na(ELV), 0, ELV)
BUIEFF <- ifelse(BUIEFF <= 0, 0, 1)
BUIEFF <- ifelse(is.na(BUIEFF), 1, BUIEFF)
HR <- ifelse(HR < 0, -HR, HR)
HR <- ifelse(HR > 366 * 24, 24, HR)
HR <- ifelse(is.na(HR), 0, HR)
FFMC <- ifelse(FFMC < 0 | FFMC > 101, 0, FFMC)
FFMC <- ifelse(is.na(FFMC), 90, FFMC)
ISI <- ifelse(is.na(ISI) | ISI < 0 | ISI > 300, 0, ISI)
BUI <- ifelse(BUI < 0 | BUI > 1000, 0, BUI)
BUI <- ifelse(is.na(BUI), 60, BUI)
WS <- ifelse(WS < 0 | WS > 300, 0, WS)
WS <- ifelse(is.na(WS), 10, WS)
WD <- ifelse(is.na(WD) | WD < -2 * pi | WD > 2 * pi, 0, WD)
GS <- ifelse(is.na(GS) | GS < 0 | GS > 200, 0, GS)
GS <- ifelse(ASPECT < -2 * pi | ASPECT > 2 * pi, 0, GS)
PC <- ifelse(is.na(PC) | PC < 0 | PC > 100, 50, PC)
PDF <- ifelse(is.na(PDF) | PDF < 0 | PDF > 100, 35, PDF)
CC <- ifelse(CC <= 0 | CC > 100, 95, CC)
CC <- ifelse(is.na(CC), 80, CC)
GFL <- ifelse(is.na(GFL) | GFL <= 0 | GFL > 100, 0.35, GFL)
LAT <- ifelse(LAT < -90 | LAT > 90, 0, LAT)
LAT <- ifelse(is.na(LAT), 55, LAT)
LONG <- ifelse(LONG < -180 | LONG > 360, 0, LONG)
LONG <- ifelse(is.na(LONG), -120, LONG)
THETA <- ifelse(is.na(THETA) | THETA < -2 * pi | THETA >
                    2 * pi, 0, THETA)
SD <- ifelse(SD < 0 | SD > 1e+05, -999, SD)
SD <- ifelse(is.na(SD), 0, SD)
SH <- ifelse(SH < 0 | SH > 100, -999, SH)
SH <- ifelse(is.na(SH), 0, SH)
FUELTYPE <- sub("-", "", FUELTYPE)
FUELTYPE <- sub(" ", "", FUELTYPE)
if (length(FUELTYPE[is.na(FUELTYPE)]) > 0) {
    warning("FuelType contains NA, using C2 (default) in the calculation")
    FUELTYPE <- ifelse(is.na(FUELTYPE), "C2", FUELTYPE)
}
HR <- HR * 60
WAZ <- WD + pi
WAZ <- ifelse(WAZ > 2 * pi, WAZ - 2 * pi, WAZ)
SAZ <- ASPECT + pi
SAZ <- ifelse(SAZ > 2 * pi, SAZ - 2 * pi, SAZ)
LONG <- ifelse(LONG < 0, -LONG, LONG)
SFC <- TFC <- HFI <- CFB <- ROS <- rep(0, length(LONG))
RAZ <- rep(-999, length(LONG))
validOutTypes <- c("SECONDARY", "ALL", "S", "A")
validOutTypes <- c(validOutTypes, c("RAZ0", "WSV0"))
if (output %in% validOutTypes) {
    FROS <- BROS <- TROS <- HROSt <- FROSt <- BROSt <- TROSt <- FCFB <- BCFB <- TCFB <- FFI <- BFI <- TFI <- FTFC <- BTFC <- TTFC <- rep(0,
                                                                                                                                         length(LONG))
    TI <- FTI <- BTI <- TTI <- LB <- WSV <- rep(-999, length(LONG))
}
CBH <- crown_base_height(FUELTYPE, CBH, SD, SH)
CFL <- crown_fuel_load(FUELTYPE, CFL)
FMC <- ifelse(FMC <= 0 | FMC > 120 | is.na(FMC), foliar_moisture_content(LAT,
                                                                                  LONG, ELV, DJ, D0), FMC)
FMC <- ifelse(FUELTYPE %in% c("D1", "S1", "S2", "S3", "O1A",
                              "O1B"), 0, FMC)
SFC <- surface_fuel_consumption(FUELTYPE, FFMC, BUI, PC,
                                GFL)
BUI <- ifelse(BUIEFF != 1, 0, BUI)
slope_values <- slope_adjustment(FUELTYPE, FFMC, BUI, WS,
                                 WAZ, GS, SAZ, FMC, SFC, PC, PDF, CC, CBH, ISI)
WSV0 <- slope_values[["WSV"]]
if ("WSV0" == output) {
    return(WSV0)
}
WSV <- ifelse(GS > 0 & FFMC > 0, WSV0, WS)
RAZ0 <- slope_values[["RAZ"]]
if ("RAZ0" == output) {
    return(RAZ0)
}
RAZ <- ifelse(GS > 0 & FFMC > 0, RAZ0, WAZ)
ISI <- ifelse(ISI > 0, ISI, initial_spread_index(FFMC, WSV,
                                                 TRUE))
ros_vars <- rate_of_spread_extended(FUELTYPE, ISI, BUI, FMC,
                                    SFC, PC, PDF, CC, CBH)
ROS <- ros_vars$ROS
CFB <- ifelse(CFL > 0, ros_vars$CFB, 0)
CSI <- ros_vars$CSI
RSO <- ros_vars$RSO
TFC <- total_fuel_consumption(FUELTYPE, CFL, CFB, SFC, PC,
                              PDF)
HFI <- fire_intensity(TFC, ROS)
CFB <- ifelse(HR < 0, -CFB, CFB)
RAZ <- RAZ * 180/pi
RAZ <- ifelse(RAZ == 360, 0, RAZ)
FD <- rep("I", length(CFB))
FD <- ifelse(CFB < 0.1, "S", FD)
FD <- ifelse(CFB >= 0.9, "C", FD)
CFC <- total_fuel_consumption(FUELTYPE, CFL, CFB, SFC, PC,
                              PDF, option = "CFC")
if (output %in% c("SECONDARY", "ALL", "S", "A")) {
    SF <- ifelse(GS >= 70, 10, exp(3.533 * (GS/100)^1.2))
    BE <- buildup_effect(FUELTYPE, BUI)
    LB <- length_to_breadth(FUELTYPE, WSV)
    LBt <- ifelse(ACCEL == 0, LB, length_to_breadth_at_time(FUELTYPE,
                                                            LB, HR, CFB))
    BROS <- back_rate_of_spread(FUELTYPE, FFMC, BUI, WSV,
                                FMC, SFC, PC, PDF, CC, CBH)
    FROS <- flank_rate_of_spread(ROS, BROS, LB)
    E <- sqrt(1 - 1/LB/LB)
    TROS <- ROS * (1 - E)/(1 - E * cos(THETA - RAZ))
    ROSt <- ifelse(ACCEL == 0, ROS, rate_of_spread_at_time(FUELTYPE,
                                                           ROS, HR, CFB))
    BROSt <- ifelse(ACCEL == 0, BROS, rate_of_spread_at_time(FUELTYPE,
                                                             BROS, HR, CFB))
    FROSt <- ifelse(ACCEL == 0, FROS, flank_rate_of_spread(ROSt,
                                                           BROSt, LBt))
    TROSt <- ifelse(ACCEL == 0, TROS, (ROSt * (1 - sqrt(1 -
                                                            1/LBt/LBt))/(1 - sqrt(1 - 1/LBt/LBt) * cos(THETA -
                                                                                                           RAZ))))
    FCFB <- ifelse(CFL == 0, 0, ifelse(FUELTYPE %in% c("C6"),
                                       0, crown_fraction_burned(FROS, RSO)))
    BCFB <- ifelse(CFL == 0, 0, ifelse(FUELTYPE %in% c("C6"),
                                       0, crown_fraction_burned(BROS, RSO)))
    TCFB <- ifelse(CFL == 0, 0, ifelse(FUELTYPE %in% c("C6"),
                                       0, crown_fraction_burned(TROS, RSO)))
    FTFC <- total_fuel_consumption(FUELTYPE, CFL, FCFB, SFC,
                                   PC, PDF)
    BTFC <- total_fuel_consumption(FUELTYPE, CFL, BCFB, SFC,
                                   PC, PDF)
    TTFC <- total_fuel_consumption(FUELTYPE, CFL, TCFB, SFC,
                                   PC, PDF)
    FFI <- fire_intensity(FTFC, FROS)
    BFI <- fire_intensity(BTFC, BROS)
    TFI <- fire_intensity(TTFC, TROS)
    HROSt <- ifelse(HR < 0, -ROSt, ROSt)
    FROSt <- ifelse(HR < 0, -FROSt, FROSt)
    BROSt <- ifelse(HR < 0, -BROSt, BROSt)
    TROSt <- ifelse(HR < 0, -TROSt, TROSt)
    a1 <- 0.115 - (18.8 * CFB^2.5 * exp(-8 * CFB))
    TI <- log(ifelse(1 - RSO/ROS > 0, 1 - RSO/ROS, 1))/(-a1)
    a2 <- 0.115 - (18.8 * FCFB^2.5 * exp(-8 * FCFB))
    FTI <- log(ifelse(1 - RSO/FROS > 0, 1 - RSO/FROS, 1))/(-a2)
    a3 <- 0.115 - (18.8 * BCFB^2.5 * exp(-8 * BCFB))
    BTI <- log(ifelse(1 - RSO/BROS > 0, 1 - RSO/BROS, 1))/(-a3)
    a4 <- 0.115 - (18.8 * TCFB^2.5 * exp(-8 * TCFB))
    TTI <- log(ifelse(1 - RSO/TROS > 0, 1 - RSO/TROS, 1))/(-a4)
    DH <- ifelse(ACCEL == 1, distance_at_time(FUELTYPE, ROS,
                                              HR, CFB), ROS * HR)
    DB <- ifelse(ACCEL == 1, distance_at_time(FUELTYPE, BROS,
                                              HR, CFB), BROS * HR)
    DF <- ifelse(ACCEL == 1, (DH + DB)/(LBt * 2), (DH + DB)/(LB *
                                                                 2))
}
if (!exists("ID") || is.null(ID)) {
    ID <- row.names(input)
}
if (output %in% c("PRIMARY", "P")) {
    FBP <- data.frame(ID, CFB, CFC, FD, HFI, RAZ, ROS, SFC,
                      TFC)
    FBP[, c(2:3, 5:ncol(FBP))] <- apply(FBP[, c(2:3, 5:ncol(FBP))],
                                        2, function(.x) {
                                            ifelse(FUELTYPE %in% c("WA", "NF"), 0, .x)
                                        })
    FBP[, "FD"] <- as.character(FBP[, "FD"])
    FBP[, "FD"] <- ifelse(FUELTYPE %in% c("WA", "NF"), "NA",
                          FBP[, "FD"])
} else if (output %in% c("SECONDARY", "S")) {
    FBP <- data.frame(ID, BE, SF, ISI, FFMC, FMC, D0, RSO,
                      CSI, FROS, BROS, HROSt, FROSt, BROSt, FCFB, BCFB,
                      FFI, BFI, FTFC, BTFC, TI, FTI, BTI, LB, LBt, WSV,
                      DH, DB, DF, TROS, TROSt, TCFB, TFI, TTFC, TTI)
    FBP[, 2:ncol(FBP)] <- apply(FBP[, 2:ncol(FBP)], 2, function(.x) {
        ifelse(FUELTYPE %in% c("WA", "NF"), 0, .x)
    })
} else if (output %in% c("ALL", "A")) {
    FBP <- data.frame(ID, CFB, CFC, FD, HFI, RAZ, ROS, SFC,
                      TFC, BE, SF, ISI, FFMC, FMC, D0, RSO, CSI, FROS,
                      BROS, HROSt, FROSt, BROSt, FCFB, BCFB, FFI, BFI,
                      FTFC, BTFC, TI, FTI, BTI, LB, LBt, WSV, DH, DB, DF,
                      TROS, TROSt, TCFB, TFI, TTFC, TTI)
    FBP[, c(2:3, 5:ncol(FBP))] <- apply(FBP[, c(2:3, 5:ncol(FBP))],
                                        2, function(.x) {
                                            ifelse(FUELTYPE %in% c("WA", "NF"), 0, .x)
                                        })
    FBP[, "FD"] <- as.character(FBP[, "FD"])
    FBP[, "FD"] <- ifelse(FUELTYPE %in% c("WA", "NF"), "NA",
                          FBP[, "FD"])
}
