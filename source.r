setwd("//wsl$/Ubuntu-20.04/mnt/data/FireSTARR")
library(data.table)
# library(raster)
library(terra)
library(lattice)
library(rasterVis)

DIR <- "../data/test_condense16"
# DIR_OUT <- "."
DIR_OUT <- Sys.getenv("HOME")
FILE_OUT <- file.path(DIR_OUT, paste0(basename(DIR), ".pdf"))
# colours <- list(
# "1"=list(r=255, g=0, b=0),
# "2"=list(r=255, g=255, b=255),
# "4"=list(r=247, g=250, b=21),
# "8"=list(r=0, g=0, b=0),
# #"16"=list(r=185, g=122, b=87),
# #"32"=list(r=34, g=177, b=76),
# #"64"=list(r=163, g=73, b=164),
# #"128"=list(r=0, g=162, b=232)
# "16"=list(r=0, g=255, b=0),
# "32"=list(r=0, g=0, b=255),
# "64"=list(r=200, g=200, b=0),
# "128"=list(r=200, g=0, b=200)
# )
# colours <- list(
#   "1"=list(r=255, g=0, b=0),
#   "2"=list(r=0, g=255, b=0),
#   "4"=list(r=0, g=0, b=255),
#   "8"=list(r=255, g=255, b=255),
#   "16"=list(r=255, g=255, b=0),
#   "32"=list(r=0, g=255, b=255),
#   "64"=list(r=255, g=0, b=255),
#   "128"=list(r=0, g=0, b=0)
# )
# colours <- list(
#   "1"=list(r=255, g=0, b=0),
#   "2"=list(r=0, g=0, b=255),
#   "4"=list(r=0, g=0, b=0),
#   "8"=list(r=255, g=255, b=255),
#   "16"=list(r=0, g=255, b=0),
#   "32"=list(r=255, g=0, b=255),
#   "64"=list(r=255, g=0, b=255),
#   "128"=list(r=0, g=255, b=255)
# )
colours <- list(
  "1" = list(r = 98, g = 45, b = 209),
  "2" = list(r = 255, g = 255, b = 0),
  "4" = list(r = 113, g = 143, b = 200),
  "8" = list(r = 255, g = 0, b = 0),
  "16" = list(r = 0, g = 0, b = 255),
  "32" = list(r = 255, g = 144, b = 0),
  "64" = list(r = 144, g = 0, b = 144),
  "128" = list(r = 144, g = 255, b = 0)
)
results <- rbind(NULL, c(Value = 0, Red = 0, Green = 0, Blue = 0))
for (i in 1:255) {
  if (!(i %in% names(colours))) {
    cur <- i
    count <- 0
    r <- 0
    g <- 0
    b <- 0
    print(i)
    for (j in 1:(i - 1)) {
      if (bitwAnd(cur, j) > 0) {
        print(paste0("Matches ", j))
        co <- colours[[as.character(j)]]
        print(co)
        r <- r + co$r
        g <- g + co$g
        b <- b + co$b
        count <- count + 1
        cur <- bitwAnd(cur, bitwNot(j))
      }
    }
    r <- as.integer(r / count)
    g <- as.integer(g / count)
    b <- as.integer(b / count)
    colours[[as.character(i)]] <- list(r = r, g = g, b = b)
  }
  v <- colours[[as.character(i)]]
  results <- rbind(results, c(Value = i, Red = v$r, Green = v$g, Blue = v$b))
}

to_hex <- function(r, g, b) {
  return(rgb(r, g, b, maxColorValue = 255))
}
hex <- list()
for (i in 1:(length(colours))) {
  v <- colours[[as.character(i)]]
  hex[as.character(i)] <- to_hex(v$r, v$g, v$b)
}
# hex <- sapply(colours,
#               function(v) {
#                 print(v)
#                 return(to_hex(v$r, v$g, v$b))
#               })
hex <- unlist(hex)
hex <- c(to_hex(0, 0, 0), hex)
names(hex) <- 0:255
results <- as.data.table(results)
write.table(results, file = "source.clr", col.names = FALSE, row.names = FALSE, sep = " ")

imgs <- list.files(DIR, pattern = "^source.tif$", full.names = TRUE, recursive = TRUE)
imgs <- imgs[grep("C2", imgs)]
rasters <- lapply(imgs, function(img) {
  return(trim(rast(img)))
})
e <- ext(rasters[[1]])
for (r in rasters) {
  e <- ext(extend(r, e))
}
pdf(FILE_OUT, width = 11, height = 8.5)
par(mfrow = c(3, 4), mar = c(2, 2, 2, 2))
for (i in 1:length(rasters)) {
  img <- imgs[[i]]
  title <- basename(dirname(img))
  r <- rasters[[i]]
  # set largest extent between all rasters
  r <- extend(r, e)
  # HACK: have to filter to values in the raster so it picks right colours
  # col <- hex[1 + unique(r)]
  # col <- hex[unique(r)]
  # col <- hex[1:max(unique(r))]
  # col <- hex[as.character(unique(r))]
  col <- hex[as.integer(unlist(unique(r)))]
  # plot(r, col=col, main=title)
  # plot(ratify(r), col=hex, main=title)
  # colortable(r) <- hex
  # plot(r, main=title)
  # print(levelplot(ratify(r), col.regions=col, att='ID'))
  r <- as.factor(r)
  # lvls <- levels(r)[[1]]
  # lvls$ID <- unique(r)
  # levels(r) <- lvls
  # print(levelplot(r, col.regions = col, att = "ID", main = title))
  plot(r, col=col, main=title)
}
dev.off()
