cat("\014")
rm(list=ls())
graphics.off()


Resolution=300



GetScriptPath <- function()
{
    argv <- commandArgs(trailingOnly = FALSE)
    if (any(grepl("--interactive", argv))) {
        GetScriptPath <-"E:/Project/Examples/InsectModellingWithBioSIM/"
    } else {
        GetScriptPath <- paste(dirname(substring(argv[grep("--file=", argv)],8)), "/../", sep='')
    }
}


GetFilePath <- function(name)
{
    GetFilePath <- paste(GetScriptPath(), name, sep='')
}

GetBioSIMOutputFilePath <- function()
{
    argv <- commandArgs(trailingOnly = FALSE)
    if (any(grepl("--interactive", argv))) { 
		 GetBioSIMOutputFilePath <- GetFilePath("output/DevTime(Best).csv")
    } else { GetBioSIMOutputFilePath <- argv[grep("--args", argv)+1] 
    }
}



dir.create(GetFilePath("Images/"), showWarnings = FALSE);

obs_file_path = GetFilePath("Input/DevTime.csv");
obs = read.csv(obs_file_path);

if(!"Rate" %in% colnames(obs) & "Time" %in% colnames(obs))
	obs$Rate = 1/obs$Time;



obs$group <- paste(obs$Variable,sprintf("%02.1f",obs$T))
obs_all <- data.frame( Variable=rep(obs$Variable, obs$n), T=rep(obs$T, obs$n), Time=rep(obs$Time, obs$n), Rate=rep(obs$Rate, obs$n),group=rep(obs$group, obs$n))
obs_all$MeanTime <- ave(obs_all$Time, obs_all$group, FUN = mean)
obs_all$TimeSD <- ave(obs_all$Time, obs_all$group, FUN = sd)
obs_all$RDT <- obs_all$Time/obs_all$MeanTime
obs_all$MeanRate <- ave(obs_all$Rate, obs_all$group, FUN = mean)
obs_all$RDR <- obs_all$Rate/obs_all$MeanRate
obs_all$qTime = ave( obs_all$RDT, obs_all$Variable,FUN=function(x){pmax( 0.001, pmin(0.999, rank(x, TRUE, "first")/(length(x)+1)))});
obs_all$qRate = ave( obs_all$qTime, obs_all$Variable,FUN=rev);#	qRate is the reverse of qTime to get exactly the same rank for ties 






sim = read.csv(GetBioSIMOutputFilePath())

variables <- c( "Egg", "Larva", "Prepupa", "Pupa") #, "Adult"
 



obs_all$qRate = ave( obs_all$RDR, obs_all$Variable,FUN=function(x){pmax( 0.001, pmin(0.999, rank(x, ties.method="first")/(length(x)+1)))});



MAX_TIME = aggregate(Time~Variable, obs, FUN=max)$Time*1.25
file_title = unlist(strsplit(basename(GetBioSIMOutputFilePath()), "\\."))[1]










png(file=GetFilePath(paste("Images/",file_title,"_final.png",sep='')), height=8.5, width=12, units = "in", res = Resolution, pointsize = 10)
par(mfrow=c(length(variables),5), mar=c(2.0, 4.0, 0.5, 0.5), oma = c(2.5, 0.5, 3.5, 2.5), cex=1.0, cex.main = 1.6, cex.lab=1.2, cex.axis=1.0)
{

for(i in 1:length(variables) )
{
	#i=4
	v <- variables[i]
	
	
	obs_allS<-obs_all[which(obs_all$Variable==as.character(v)),];
	obsS<-obs[which(obs$Variable==as.character(v)),];
	simS<-sim[which(sim$Variable==v),];
	
	P <- unlist(strsplit(as.character(simS$P), " "))
	Param <- strsplit(as.character(P), "=")
	for (j in 1:length(Param))
		assign(Param[[j]][1], as.double(Param[[j]][2]) )
	L <- lapply(Param, '[[', 1)	
	E <- parse(text = as.character(simS$Eq))
	
	

	T = obs_allS$T;
	Rate <- pmin(1.2, pmax(0.001, eval(E)))
	RDR <- qlnorm(obs_allS$qRate,-0.5*sigma^2,sigma)
	obs_allS$simRate=Rate*RDR
	obs_allS$simTime=1/obs_allS$simRate
	
	
	T <- seq(0,35,0.5)
	Rate <- pmin(1.2, pmax(0.001, eval(E)))
	Rate_01 = Rate*qlnorm(0.01,-0.5*sigma^2,sigma)
	Rate_99 = Rate*qlnorm(0.99,-0.5*sigma^2,sigma)
	
#1###############################################################################################
	xLim = range(T)
	yLim<-range(obsS$Rate, Rate, Rate_01, Rate_99, na.rm=TRUE)
	yLim[2] = min(1.2, yLim[2] * 1.3)
	
	plot( NA, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim))
	axis(2, at = pretty(yLim), las=TRUE)
	mtext(side=1, text=if(!i%%length(variables))"Temperature (캜)" else "", line=2.5, cex = par()$cex.lab)
	
	
	
	
	polygon(c(T, rev(T)), c(Rate_99, rev(Rate_01)), col=gray(0.95), border=NA, lwd=1, lty = 1)
	lines( Rate~T, lwd=2.5, col=gray(0.5))
	points( obsS$Rate~obsS$T, pch=21, col = 'white', bg='black', lwd=0.85, cex=par()$cex*0.9)
	
	if("R2" %in% colnames(simS)){
		t1 = sprintf("R� = %.3f", simS$R2 )
		t2 = ""
	}else {
		t1 = sprintf("maxLL = %.1f",simS$maxLL)
		t2 = sprintf("AICc = %.1f", simS$AICc )
	}
	
	name = strsplit(simS$EqName, "_")[[1]][1]
	year = strsplit(simS$EqName, "_")[[1]][2]
	if(is.na(year))year="" else year=paste(" (",year,")",sep="");

	legend('topleft',legend=c(as.character(paste(name,year,sep=" "))), inset = c(-0.125, -0.05), bty = "n", cex = par()$cex*0.90) 
	legend('topleft',legend=c(t1,t2), inset = c(-0.05, 0.05), bty = "n", cex = par()$cex*0.80) 
	if(!is.na(match("Tb", unlist(L))))
		text(Tb+4, (yLim[2]-yLim[1])/50, labels=round(Tb,1), col='blue', cex = par()$cex*0.9)
	if(!is.na(match("Tm", unlist(L))))
		text(Tm-3, (yLim[2]-yLim[1])/50, labels=round(Tm,1), col='red', cex = par()$cex*0.9)


#2###############################################################################################
	Time =  1/Rate;
	Time_01 = pmin(MAX_TIME[i], 1/Rate_99);
	Time_99 = pmin(MAX_TIME[i], 1/Rate_01);
	
	
	yLim<-range(0,MAX_TIME[i])
	
	plot( NA, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim))
	axis(2, at = pretty(yLim), las=TRUE)
	mtext(side=1, text=if(!i%%length(variables))"Temperature (캜)" else "", line=2.5, cex = par()$cex.lab)
	
	
	polygon(c(T, rev(T)), c(Time_99, rev(Time_01)), col=gray(0.95), border=NA, lwd=1, lty = 1)
	lines( Time~T, lwd=2.5, col=gray(0.5))
	points( obsS$Time~obsS$T, pch=21, col = 'white', bg='black', lwd=0.85, cex=par()$cex*0.9)
#3###############################################################################

	xLim <- range(obs_all$RDT)

#compute simul RDT, PDF and CDF	
	RDT <- c(seq(xLim[1],xLim[2],length.out = 100))
	CDF = plnorm(RDT,-0.5*sigma^2,sigma)
	PDF <- dlnorm(RDT,-0.5*sigma^2,sigma)

#compute bins	
	nbBreaks = min(15,floor(nrow(obs_allS)/3));
	breaks = seq(xLim[1],xLim[2],length.out = nbBreaks)
	bins <- hist(obs_allS$RDT, breaks=breaks, plot=FALSE);
	
	
	yLim = range(PDF, bins$density)
	yLim[2] = yLim[2]*1.3
	plot( NA, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim))
	axis(2, at = pretty(yLim), las=TRUE)
	mtext(side=1, text=if(!i%%length(variables))"Relative Development Time" else "", line=2.5, cex = par()$cex.lab)

	lines(bins, freq=FALSE, col=gray(0.98), border=gray(0.90))
	lines( PDF~RDT, col=gray(0.85), lty=1, lwd=1.5)


	yLim2 = c(0,1)
	par(new=TRUE)
	plot( NA, main="", xlim=xLim, ylim=yLim2, xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(4, at = pretty(yLim2), labels=FALSE)

	lines(CDF~RDT, col=gray(0.4), lty=1, lwd=1)

	#subset only some points
	pos = c(seq(1, nrow(obs_allS),  ceiling(nrow(obs_allS)/nbBreaks)),nrow(obs_allS))
	pos = match(pos, rank(obs_allS$RDT, TRUE, "first"))
	lines(qTime~RDT, obs_allS[order(obs_allS$RDT),], col='black', lty=3, lwd=1)
	points(qTime~RDT, obs_allS[pos,], pch=21, col = 'white', bg='black', lwd=0.85, cex=par()$cex*0.8)

	legend('topleft',legend=bquote(sigma~"="~.(round(sigma,3))),inset=c(-0.1,-0.05), bty = "n", cex = par()$cex*0.9) 
	
#4###############################################################################
	
	obs_allS.lm <- lm(obs_allS$Time~obs_allS$simTime)
	obs_allS.lm.R2 <- summary(obs_allS.lm)$r.squared 
	obs_allS.res <- obs_allS$Time - obs_allS$simTime
	obs_allS.anova = anova(obs_allS.lm)
	
	xLim<-range(obs_allS$Time,obs_allS$simTime)
	yLim<-xLim
	plot( NA, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim))
	axis(2, at = pretty(yLim), las=1)
	
	lines( c(xLim[1],xLim[2]), c(xLim[1],xLim[2]), col=gray(0.7), lwd=3, lty = 1)
	abline(v=pretty(xLim), h=pretty(yLim), col=gray(0.95), lwd=1, lty = 1)
	points( obs_allS$Time~obs_allS$simTime, pch=21, col = 'white', bg='black', lwd=0.85, cex=par()$cex*0.9)
	
	t1 = sprintf("n = %d",length(obs_allS$Time[!is.na(obs_allS$Time)]))
	t2 = sprintf("Bias = %.2g",mean(obs_allS$simTime - obs_allS$Time, na.rm=TRUE))
	t3 = sprintf("MAE = %.2g",mean( abs(obs_allS$simTime - obs_allS$Time), na.rm=TRUE ) )
	t4 = sprintf("R� = %.3f",mean(obs_allS.lm.R2, na.rm=TRUE) )
	t5 = sprintf("F %d,%d = %.1f", obs_allS.anova[1,"Df"], obs_allS.anova[2, "Df"], obs_allS.anova[1, "F value"])
	t6 = ifelse(obs_allS.anova[1,"Pr(>F)"] < 0.001, "p.val < 0.001", sprintf("p.val = %.3f",  obs_allS.anova[1,"Pr(>F)"] ));

	legend('topleft',legend=c(t1,t2,t3,t4), inset = c(-0.1, -0.05), bty = "n", cex = par()$cex*0.8)
	legend('bottomrigh',legend=c(t5,t6), inset = c(0, 0), bty = "n", cex = par()$cex*0.8)
	
	#legend('bottomrigh',legend=c(t2,t3,t4,t5,t6), bty = "n", cex = par()$cex*0.8)
	mtext(side=1, text=if(!i%%length(variables))"Simulated Time (days)"else "", line=2.5, cex = par()$cex.lab)
	
#5###############################################################################
	obs_allS.lm <- lm(obs_allS$Rate~obs_allS$simRate)
	obs_allS.lm.R2 <- summary(obs_allS.lm)$r.squared 
	obs_allS.res <- obs_allS$Rate - obs_allS$simRate
	obs_allS.anova = anova(obs_allS.lm)

	xLim<-range(obs_allS$Rate,obs_allS$simRate)
	yLim<-xLim
	plot( NA, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim))
	axis(2, at = pretty(yLim), las=1)
	mtext(side=1, text=if(!i%%length(variables))"Simulated Rate (1/days)"else"", line=2.5, cex = par()$cex.lab)
	mtext(side=4, text=v, line=0.5, cex = par()$cex.main, las=0)

	
	lines( c(xLim[1],xLim[2]), c(xLim[1],xLim[2]), col=gray(0.7), lwd=3, lty = 1)
	abline(v=pretty(xLim), h=pretty(yLim), col=gray(0.95), lwd=1, lty = 1)
	points( obs_allS$Rate~obs_allS$simRate, pch=21, col = 'white', bg='black', lwd=0.85, cex=par()$cex*0.9)
	
	t1 = sprintf("n = %d",length(obs_allS$Rate[!is.na(obs_allS$Rate)]))
	t2 = sprintf("Bias = %.2g",mean(obs_allS$simRate - obs_allS$Rate, na.rm=TRUE))
	t3 = sprintf("MAE = %.2g",mean( abs(obs_allS$simRate - obs_allS$Rate), na.rm=TRUE ) )
	t4 = sprintf("R� = %.3f",mean(obs_allS.lm.R2, na.rm=TRUE) )
	t5 = sprintf("F %d,%d = %.1f", obs_allS.anova[1,"Df"], obs_allS.anova[2, "Df"], obs_allS.anova[1, "F value"])
	t6 = ifelse(obs_allS.anova[1,"Pr(>F)"] < 0.001, "p.val < 0.001", sprintf("p.val = %.3f",  obs_allS.anova[1,"Pr(>F)"] ));
	legend('topleft',legend=c(t1,t2,t3,t4), inset = c(-0.1, -0.05), bty = "n", cex = par()$cex*0.8)
	legend('bottomrigh',legend=c(t5,t6), bty = "n", cex = par()$cex*0.8)
	
	
################################################################################
	rm(list=unlist(L))
	}
	
	
}

mtext(side=2, text="Development Rate (1/days)", outer=TRUE, line=-1.0, cex = par()$cex.lab)
mtext(side=2, text="Development Time (days)", outer=TRUE, line=-15.25, cex = par()$cex.lab)
mtext(side=2, text="Density Distribution", outer=TRUE, line=-29.5, cex = par()$cex.lab)
mtext(side=2, text="Observed Time (days)", outer=TRUE, line=-43.5, cex = par()$cex.lab)
mtext(side=2, text="Observed Rate (1/days)", outer=TRUE, line=-57, cex = par()$cex.lab)
mtext(bquote(italic(Insect.~name)*": development"), outer = TRUE, line=0.7, cex = par()$cex.main)

dev.off()



















png(file=GetFilePath(paste("Images/",file_title,"_publication1.png",sep='')), height=8.5, width=7.3, units = "in", res = Resolution, pointsize = 11)
par(mfrow=c(length(variables),3), mar=c(2.0, 3.5, 0.5, 0.5), oma = c(2.0, 1.5, 1.25, 1.5), cex=1.0, cex.main = 1.4, cex.lab=1.1, cex.axis=1.0)
{

for(i in 1:length(variables) )
{
	v <- variables[i]
	
	obs_allS<-obs_all[which(obs_all$Variable==as.character(v)),];
	obsS<-obs[which(obs$Variable==as.character(v)),];
	simS<-sim[which(sim$Variable==v),];
	
	P <- unlist(strsplit(as.character(simS$P), " "))
	Param <- strsplit(as.character(P), "=")
	for (j in 1:length(Param))
		assign(Param[[j]][1], as.double(Param[[j]][2]) )
	L <- lapply(Param, '[[', 1)	
	E <- parse(text = as.character(simS$Eq))

	T = obs_allS$T;
	Rate <- pmin(1.2, pmax(0.001, eval(E)))
	RDR <- qlnorm(obs_allS$qRate,-0.5*sigma^2,sigma)
	obs_allS$simRate=Rate*RDR
	obs_allS$simTime=1/obs_allS$simRate
	
	
	T <- seq(0,35,0.5)
	Rate <- pmin(1.2, pmax(0.001, eval(E)))
	Rate_01 = Rate*qlnorm(0.01,-0.5*sigma^2,sigma)
	Rate_99 = Rate*qlnorm(0.99,-0.5*sigma^2,sigma)
	
#1###############################################################################################
	
	xLim = range(T)
	yLim<-range(obsS$Rate, Rate, Rate_01, Rate_99, na.rm=TRUE)
	#yLim[2] = min(1.2, yLim[2] * 1.4)

	plot( NA, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim))
	axis(2, at = pretty(yLim), las=TRUE)
	mtext(side=1, text=if(!i%%length(variables))"Temperature (캜)" else "", line=2.5, cex = par()$cex.lab)
	mtext(side=3, text=if(i==1)"a" else "", line=0.25, cex = par()$cex.lab);		
	
	
	
	polygon(c(T, rev(T)), c(Rate_99, rev(Rate_01)), col=gray(0.95), border=NA, lwd=1, lty = 1)
	lines( Rate~T, lwd=2.0, col=gray(0.5))
	points( obsS$Rate~obsS$T, pch=21, col = 'white', bg='black', lwd=0.85, cex=par()$cex*0.8)
	
	#t1 = sprintf("maxLL = %.1f",simS$maxLL)
	#t2 = sprintf("AICc = %.1f", simS$AICc )
	#legend('topleft',legend=c(t1,t2), inset = c(-0.05, 0.0), bty = "n", cex = par()$cex*0.9) 
	
	
	if(!is.na(match("Tb", unlist(L))))
		text(Tb+4.5, (yLim[2]-yLim[1])/50, labels=round(Tb,1), col='blue', cex = par()$cex*0.9)
	if(!is.na(match("Tm", unlist(L))))
		text(Tm-3, (yLim[2]-yLim[1])/50, labels=round(Tm,1), col='red', cex = par()$cex*0.9)

	abline( lm(Rate~T, obs_allS), lty=5, lwd=1.0, col=gray(0.25))
#2###############################################################################

	
	xLim <- range(obs_all$RDT)

#compute simul RDT, PDF and CDF	
	RDT <- c(seq(xLim[1],xLim[2],length.out = 100))
	CDF = plnorm(RDT,-0.5*sigma^2,sigma)
	PDF <- dlnorm(RDT,-0.5*sigma^2,sigma)

#compute bins	
	nbBreaks = min(15,floor(nrow(obs_allS)/3));
	breaks = seq(xLim[1],xLim[2],length.out = nbBreaks)
	bins <- hist(obs_allS$RDT, breaks=breaks, plot=FALSE);
	
	
	yLim = range(PDF, bins$density)
	yLim[2] = yLim[2]*1.3
	plot( NA, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim))
	#axis(4, at = pretty(yLim), labels=FALSE)
	mtext(side=1, text=if(!i%%length(variables))"Relative Development Time" else "", line=2.5, cex = par()$cex.lab)
	mtext(side=3, text=if(i==1)"b" else "", line=0.25, cex = par()$cex.lab);

	lines(bins, freq=FALSE, col=gray(0.98), border=gray(0.90))
	lines( PDF~RDT, col=gray(0.85), lty=1, lwd=1.5)


	yLim2 = c(0,1)
	par(new=TRUE)
	plot( NA, main="", xlim=xLim, ylim=yLim2, xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(2, at = pretty(yLim2), las=1)
	#axis(4, at = pretty(yLim2), labels=FALSE)

	lines(CDF~RDT, col=gray(0.4), lty=1, lwd=1)

	#subset only some points
	pos = c(seq(1, nrow(obs_allS),  ceiling(nrow(obs_allS)/nbBreaks)),nrow(obs_allS))
	pos = match(pos, rank(obs_allS$RDT, TRUE, "first"))
	lines(qTime~RDT, obs_allS[order(obs_allS$RDT),], col='black', lty=3, lwd=1)
	points(qTime~RDT, obs_allS[pos,], pch=21, col = 'white', bg='black', lwd=0.85, cex=par()$cex*0.8)

	legend('topleft',legend=bquote(sigma~"="~.(round(sigma,3))),inset=c(-0.1,-0.05), bty = "n", cex = par()$cex*0.9) 
	
	
	
#3###############################################################################
	
	obs_allS.lm <- lm(obs_allS$Time~obs_allS$simTime)
	obs_allS.lm.R2 <- summary(obs_allS.lm)$r.squared 
	obs_allS.res <- obs_allS$Time - obs_allS$simTime
	obs_allS.anova = anova(obs_allS.lm)

	
	xLim<-range(obs_allS$Time,obs_allS$simTime)
	yLim<-xLim
	plot( NA, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim))
	axis(2, at = pretty(yLim), las=1)
	mtext(side=1, text=if(!i%%length(variables))"Simulated Time (days)"else "", line=2.5, cex = par()$cex.lab)
	mtext(side=3, text=if(i==1)"c" else "", line=0.25, cex = par()$cex.lab);		
	mtext(side=4, text=v, line=0.5, cex = par()$cex.main, las=0)
	
	
	
	lines( c(xLim[1],xLim[2]), c(xLim[1],xLim[2]), col=gray(0.7), lwd=3, lty = 1)
	abline(v=pretty(xLim), col=gray(0.95), lwd=1, lty = 1)
	abline(h=pretty(yLim), col=gray(0.95), lwd=1, lty = 1)
	points( obs_allS$Time~obs_allS$simTime, pch=21, col = 'white', bg='black', lwd=0.85, cex=par()$cex*0.8)
	
	t1 = sprintf("n = %d",length(obs_allS$Time[!is.na(obs_allS$Time)]))
	t2 = sprintf("Bias = %.2g",mean(obs_allS$simTime - obs_allS$Time, na.rm=TRUE))
	t3 = sprintf("MAE = %.2g",mean( abs(obs_allS$simTime - obs_allS$Time), na.rm=TRUE ) )
	t4 = sprintf("R� = %.3f",mean(obs_allS.lm.R2, na.rm=TRUE) )
	t5 = sprintf("F %d,%d = %.1f", obs_allS.anova[1,"Df"], obs_allS.anova[2, "Df"], obs_allS.anova[1, "F value"])
	t6 = ifelse(obs_allS.anova[1,"Pr(>F)"] < 0.001, "p.val < 0.001", sprintf("p.val = %.3f",  obs_allS.anova[1,"Pr(>F)"] ));

	legend('topleft',legend=c(t1,t2,t3,t4), inset = c(-0.1, -0.05), bty = "n", cex = par()$cex*0.8)
	legend('bottomrigh',legend=c(t5,t6), inset = c(0, 0), bty = "n", cex = par()$cex*0.8)
	
################################################################################
	rm(list=unlist(L))
	}
	
	
}

mtext(side=2, text="Development Rate (1/days)", outer=TRUE, line=0, cex = par()$cex.lab)
mtext(side=2, text="Density Distribution", outer=TRUE, line=-13.25, cex = par()$cex.lab)
mtext(side=2, text="Observed Time (days)", outer=TRUE, line=-25.75, cex = par()$cex.lab)

dev.off()










png(file=GetFilePath(paste("Images/",file_title,"_publication2.png",sep='')), height=8.5, width=7.3, units = "in", res = Resolution, pointsize = 11)
par(mfrow=c(length(variables),3), mar=c(2.0, 3.5, 0.5, 0.5), oma = c(2.0, 1.5, 1.25, 1.5), cex=1.0, cex.main = 1.4, cex.lab=1.1, cex.axis=1.0)
{

for(i in 1:length(variables) )
{
	v <- variables[i]
	
	obs_allS<-obs_all[which(obs_all$Variable==as.character(v)),];
	obsS<-obs[which(obs$Variable==as.character(v)),];
	simS<-sim[which(sim$Variable==v),];
	
	P <- unlist(strsplit(as.character(simS$P), " "))
	Param <- strsplit(as.character(P), "=")
	for (j in 1:length(Param))
		assign(Param[[j]][1], as.double(Param[[j]][2]) )
	L <- lapply(Param, '[[', 1)	
	E <- parse(text = as.character(simS$Eq))
	
	

	T = obs_allS$T;
	Rate <- pmin(1.2, pmax(0.001, eval(E)))
	RDR <- qlnorm(obs_allS$qRate,-0.5*sigma^2,sigma)
	obs_allS$simRate=Rate*RDR
	obs_allS$simTime=1/obs_allS$simRate
	
	
	T <- seq(0,35,0.5)
	Rate <- pmin(1.2, pmax(0.001, eval(E)))
	Rate_01 = Rate*qlnorm(0.01,-0.5*sigma^2,sigma)
	Rate_99 = Rate*qlnorm(0.99,-0.5*sigma^2,sigma)
	
#1###############################################################################################
	
	xLim = range(T)
	yLim<-range(obsS$Rate, Rate, Rate_01, Rate_99, na.rm=TRUE)
	#yLim[2] = min(1.2, yLim[2] * 1.4)
	
	plot( NA, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim))
	axis(2, at = pretty(yLim), las=TRUE)
	mtext(side=1, text=if(!i%%length(variables))"Temperature (캜)" else "", line=2.5, cex = par()$cex.lab)
	mtext(side=3, text=if(i==1)"a" else "", line=0.25, cex = par()$cex.lab);		
	
	
	
	polygon(c(T, rev(T)), c(Rate_99, rev(Rate_01)), col=gray(0.95), border=NA, lwd=1, lty = 1)
	lines( Rate~T, lty=1, lwd=2.0, col=gray(0.5))
	points( obsS$Rate~obsS$T, pch=21, col = 'white', bg='black', lwd=0.85, cex=par()$cex*0.8)

	abline( lm(Rate~T, obs_allS), lty=5, lwd=1.0, col=gray(0.25))
	
#2##############################################################################################
	Time =  1/Rate;
	Time_01 = pmin(MAX_TIME[i], 1/Rate_99);
	Time_99 = pmin(MAX_TIME[i], 1/Rate_01);
	
	
	yLim<-range(0,MAX_TIME[i])
	
	plot( NA, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim))
	axis(2, at = pretty(yLim), las=1)
	mtext(side=1, text=if(!i%%length(variables))"Temperature (캜)" else "", line=2.5, cex = par()$cex.lab)
	mtext(side=3, text=if(i==1)"b" else "", line=0.25, cex = par()$cex.lab);		
	
	cex = 0.8#pmax(0.8, 1.8*log(obsS$n)/max(log(obsS$n)))
	polygon(c(T, rev(T)), c(Time_99, rev(Time_01)), col=gray(0.95), border=NA, lwd=1, lty = 1)
	#lines( Time~T, lwd=2.5, col=gray(0.5))
	lines( Time~T, lty=1, lwd=2.0, col=gray(0.5))
	points( obsS$Time~obsS$T, pch=21, col = 'white', bg='black', lwd=0.85, cex=par()$cex*0.8)
	
#3###############################################################################

	xLim = range(obs_all$RDT)
	yLim = c(0,1)

	CDF = plnorm(RDT,-0.5*sigma^2,sigma)
	
	
	plot( NA, main="", xlim=xLim, ylim=yLim, xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim))
	axis(2, at = pretty(yLim), las=1)
	mtext(side=1, text=if(!i%%length(variables))"Relative Development Time" else "", line=2.5, cex = par()$cex.lab)
	mtext(side=3, text=if(i==1)"c" else "", line=0.25, cex = par()$cex.lab);		

	lines(CDF~RDT, col=gray(0.4), lty=1, lwd=1)

	#subset only some points
	pos = c(seq(1, nrow(obs_allS),  ceiling(nrow(obs_allS)/nbBreaks)),nrow(obs_allS))
	pos = match(pos, rank(obs_allS$RDT, TRUE, "first"))
	lines(qTime~RDT, obs_allS[order(obs_allS$RDT),], col='black', lty=3, lwd=1)
	points(qTime~RDT, obs_allS[pos,], pch=21, col = 'white', bg='black', lwd=0.85, cex=par()$cex*0.8)
	
################################################################################
	rm(list=unlist(L))
	}
	
	
}

mtext(side=2, text="Development Rate (1/days)", outer=TRUE, line=0, cex = par()$cex.lab)
mtext(side=2, text="Development Time (days)", outer=TRUE, line=-13.5, cex = par()$cex.lab)
mtext(side=2, text="Density Distribution", outer=TRUE, line=-25.5, cex = par()$cex.lab)




dev.off()







