cat("\014")
rm(list=ls())
graphics.off()


GetScriptPath <- function()
{
    
    if (interactive()) {
        GetScriptPath <-"D:/Project/Examples/Demo BioSIM/Script/"
    } else {
        argv <- commandArgs(trailingOnly = FALSE)
        GetScriptPath <- paste(dirname(substring(argv[grep("--file=", argv)],8)), "/", sep='')
    }
}


GetFilePath <- function(name)
{
    GetFilePath <- paste(GetScriptPath(), name, sep='')
}


GetBioSIMOutputFilePath <- function()
{
    if (interactive()) { 
         GetBioSIMOutputFilePath <- GetFilePath("../output/SBW_TimeSeries.csv")
    } else { 
        argv <- commandArgs(trailingOnly = FALSE)
        GetBioSIMOutputFilePath <- argv[grep("--args", argv)+1] 
    }
}


dir.create(GetFilePath("../Images/"), showWarnings = FALSE)
Esim <- read.csv(GetBioSIMOutputFilePath())
str(Esim)

#use only the first year of the first location
Esim <- Esim[ Esim[,1]==unique(Esim[,1])[1]&Esim[,2]==unique(Esim[,2])[1],]


Esim$Date <- paste(Esim$Year,formatC(Esim$Month, width=2, flag="0"),formatC(Esim$Day, width=2, flag="0"), sep="-")
Esim$date <- as.POSIXct(Esim$Date)

#select variable to plot
variables <- c("L2","L3","L4","L5","L6","Pupae","Adults");


Resolution=300
png(file=GetFilePath("../Images/TimeSeries.png"), height=11, width=8.5, units = "in", res = Resolution, pointsize = 10)
#                       bott,left,top,righ
par(mfcol=c(1,1), mar=c(2.5, 4, 3, 1), oma = c(0, 0, 0, 0), font.main=1, cex.main = 1.5, cex=2, cex.lab=1.2)

	yLim = range(Esim[,variables])
	test1<-Esim[,variables]>0.01
	test2 <- apply(test1, 1, function(x) {any(x)})
	xLim = range(Esim[test2,]$date)

	
	color <- rainbow(length(variables)+1, alpha=0.75)

	plot(NA, type="n", las=TRUE, xlim=xLim, ylim=yLim, xlab="", ylab="", frame=FALSE , xaxt = "n" )
	dates_pos <- seq(min(xLim), max(xLim), by="weeks")
	axis(1, dates_pos, format(dates_pos, "%b %d"), cex.axis = 1.0)
	for(v in c(1:length(variables)))
	{
		lines(Esim[,variables[v]]~Esim$date, col=color[v],lwd=5, lty = (v-1)%%3+1)
	}
	
	legend('topright', legend=c(variables), lty=(c(1:length(variables))-1)%%3+1, lwd=5, col=color, bty = "n", cex=1.25, seg.len=3) 
	mtext("SBW stage [%]", 2,cex=3, line=2.5) 
	mtext(paste("SBW stage,",unique(Esim[,1]), unique(Esim[,2])), 3, cex=3, line=1) 

# Turn off device driver (to flush output to image)
dev.off()

