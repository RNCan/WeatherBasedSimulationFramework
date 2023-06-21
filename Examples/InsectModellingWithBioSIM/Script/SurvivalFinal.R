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
		 GetBioSIMOutputFilePath <- GetFilePath("output/Survival(Best).csv")
    } else { GetBioSIMOutputFilePath <- argv[grep("--args", argv)+1] 
    }
}


dir.create(GetFilePath("Images/"), showWarnings = FALSE)


obs = read.csv(GetFilePath("Input/Survival.csv"), na.string="NA")


#overall survival
if(!"S" %in% colnames(obs))
	obs$S = obs$Survival/obs$n

#daily survival
if(!"s" %in% colnames(obs))
	obs$s = obs$S^(1/obs$MeanTime)


Survived = c()
for( i in 1:nrow(obs))
{
	Survived = c(Survived, rep(0, obs$dead[i]))
	Survived = c(Survived, rep(1, obs$Survival[i]))
}


obs$group = paste(obs$Variable,sprintf("%02.1f",obs$T))
obs_all = data.frame( Variable=rep(obs$Variable, obs$n), T=rep(obs$T, obs$n), MeanTime=rep(obs$MeanTime, obs$n), Survival=Survived,s=rep(obs$s, obs$n),S=rep(obs$S, obs$n),group=rep(obs$group, obs$n))


#load survival simulation
sim = read.csv(GetBioSIMOutputFilePath())
#load development simulation
dev_rate = read.csv(GetFilePath("output/DevTime(Best).csv"))


#MAX_TIME = c(80,60,50,40);
variables = unique(sim$Variable)
file_title = unlist(strsplit(basename(GetBioSIMOutputFilePath()), "\\."))[1]


png(file=GetFilePath(paste("Images/",file_title,"_final.png",sep='')), height=8.5, width=8.5, units = "in", res = Resolution, pointsize = 11)
par(mfrow=c(length(variables),3), mar=c(1.0, 4.5, 0.5, 0.5), oma = c(3.5, 0.5, 2.5, 2.5), cex=1.0, cex.main = 1.5, cex.lab=1.2, cex.axis=1.0)

for( i in 1:length(variables) )
{
	#i=1
	v <- variables[i]
	print(v)
	simS<-sim[which(sim$Variable==v),];
	obsS<-obs[which(obs$Variable==as.character(v)),];

	#compute development time
	dev_rateS<-dev_rate[which(dev_rate$Variable==v),];
	P <- unlist(strsplit(as.character(dev_rateS$P), " "))
	Param <- strsplit(as.character(P), "=")
	for (j in 1:length(Param))
		assign(Param[[j]][1], as.double(Param[[j]][2]) )
	L <- lapply(Param, '[[', 1)	
	E <- parse(text = as.character(dev_rateS$Eq))
	
	T<-obsS$T
	obsS$sim_Time <- 1/pmin(1.2, pmax(0.001, eval(E)))
	obsS$sim_S <- obsS$s^obsS$sim_Time
	
	T <- seq(0,35,0.1)
	Rate <- pmin(1.2, pmax(0.001, eval(E)))
	MeanTime = 1/Rate
	LimitedMeanTime = MeanTime
	LimitedMeanTime[Rate<=0.01] = NA;
	
	Time_01 = LimitedMeanTime*qlnorm(0.01,-0.5*sigma^2,sigma)
	Time_99 = LimitedMeanTime*qlnorm(0.99,-0.5*sigma^2,sigma)
		
	rm(list=unlist(L))
	
	
	#compute survival
	P <- unlist(strsplit(as.character(simS$P), " "))
	Param <- strsplit(as.character(P), "=")
	for (j in 1:length(Param))
		assign(Param[[j]][1], as.double(Param[[j]][2]) )
	L <- lapply(Param, '[[', 1)	
	E<-parse(text = as.character(simS$Eq))

	s = pmin(1, pmax(0, eval(E)))
	S = s^MeanTime;
	
	
	rm(list=unlist(L))

	#compute survival deviation
	N=sum(obsS$n)
	lambda = S*N
	sigma = sqrt(lambda)
	Si_01 = pmax(0.001, qnorm(0.01, lambda, sigma))
	Si_99 = pmax(0.001, qnorm(0.99, lambda, sigma))
	
	S_01 = pmax(0.001, pmin(1, Si_01/N));
	S_99 = pmax(0.001, pmin(1, Si_99/N));
	s_01 = S_01^(1/LimitedMeanTime);
	s_99 = pmin(1,S_99^(1/LimitedMeanTime));
	
#1###############################################################################
	xLim = range(T)
	yLim = extendrange(range(obsS$s, na.rm=TRUE),f=1.6)
	
	plot( NA, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim), labels = i==4 )
	axis(2, at = pretty(pmax(0,pmin(1,yLim))), labels = TRUE, las=TRUE)
	

	polygon(c(T[!is.na(s_99)], rev(T[!is.na(s_01)])), c(s_99[!is.na(s_99)], rev(s_01[!is.na(s_01)])), col=gray(0.95), border=NA, lwd=1, lty = 1)
	cex = pmax(0.8, 1.8*log(obsS$n)/max(log(obsS$n)))
	lines( s~T, lwd=2.5, col=gray(0.5))
	points( obsS$s~obsS$T, pch=21, col = 'white', bg='black', lwd=1, cex=cex)

	t1 = sprintf("maxLL = %.1f",simS$maxLL)
	t2 = sprintf("AICc = %.1f", simS$AICc )

	legend('topright',legend=as.character(simS$EqName), inset = c(0.1, -0.05),  bty = "n", cex = par()$cex*0.9) 
	legend('topright',legend=c(t1,t2), inset = c(0.0, 0.05), bty = "n", cex = par()$cex*0.8)

#2###############################################################################

	xLim = range(T)
	yLim = range(0, max(LimitedMeanTime, na.rm=TRUE))

	plot( NA, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim), labels = i==4)
	axis(2, at = pretty(yLim), las=TRUE)
	mtext(side=1, text=if(i==4)"Temperature (°C)" else "", line=2.5, cex = par()$cex.lab)

	polygon(c(T[!is.na(Time_99)], rev(T[!is.na(Time_01)])), c(Time_99[!is.na(Time_99)], rev(Time_01[!is.na(Time_01)])), col=gray(0.95), border=NA, lwd=1, lty = 1)
	cex = pmax(0.8, 1.8*log(obsS$n)/max(log(obsS$n)))
	lines( MeanTime~T, lwd=2.5, col=gray(0.5))
	points( obsS$MeanTime~obsS$T, pch=21, col = 'white', bg='black', lwd=1, cex=cex)

#3###############################################################################

	obsS.lm <- lm(obsS$S~obsS$sim_S, weights=obsS$n)
	obsS.lm.R2 <- summary(obsS.lm)$r.squared 
	obsS.anova = anova(obsS.lm)
	
	xLim = range(T)
	yLim <- range(0,1.2)
	plot( NA, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim), labels = i==4 )
	axis(2, at = pretty(range(0,1)), las=TRUE)
	
	polygon(c(T[!is.na(S_99)], rev(T[!is.na(S_01)])), c(S_99[!is.na(S_99)], rev(S_01[!is.na(S_01)])), col=gray(0.95), border=NA, lwd=1, lty = 1)
	lines( S~T, lwd=2.5, col=gray(0.5))
	lines( S~T, lwd=2.5, col=gray(0.5))
	points( obsS$S~obsS$T, pch=21, col = 'white', bg='black', lwd=1, cex=cex)

	t1 = sprintf("n = %d",length(obsS$S[!is.na(obsS$S)]))
	t4 = sprintf("R² = %.3f",mean(obsS.lm.R2, na.rm=TRUE) )
	t5 = sprintf("F %d,%d = %.1f", obsS.anova[1,"Df"], obsS.anova[2, "Df"], obsS.anova[1, "F value"])
	t6 = ifelse(obsS.anova[1,"Pr(>F)"] < 0.001, "p.val < 0.001", sprintf("p.val = %.3f",  obsS.anova[1,"Pr(>F)"] ));

	legend('topleft',legend=c(t1,t4), inset = c(-0.1, -0.05), bty = "n", cex = par()$cex*0.8)
	legend('toprigh',legend=c(t5,t6), inset = c(0.0, -0.05), bty = "n", cex = par()$cex*0.8)
	
	
	
	mtext(side=4, text=v, line=0.5, cex = par()$cex.main, las=0)

#######################


}

mtext(side=2, text="Daily Survival", outer=TRUE, line=-1.25, cex = par()$cex.lab)
mtext(side=2, text="Development Time (days)", outer=TRUE, line=-15.5, cex = par()$cex.lab)
mtext(side=2, text="Overall Survival", outer=TRUE, line=-30.0, adj=c(0.5,0.5), padj=c(0.5,0.5), cex = par()$cex.lab)
mtext(bquote(italic(Insect.~name)*": survival"), outer = TRUE, line=0.7, cex = par()$cex.main)


dev.off()









###############################################################################################





png(file=GetFilePath(paste("Images/",file_title,"_publication1.png",sep='')), height=8.5, width=8.5, units = "in", res = Resolution, pointsize = 11)
par(mfrow=c(length(variables),3), mar=c(1.0, 4.5, 0.5, 0.5), oma = c(3.5, 0.5, 2.5, 2.5), cex=1.0, cex.main = 1.5, cex.lab=1.2, cex.axis=1.0)

for( i in 1:length(variables) )
{
	#i=1
	v <- variables[i]
	print(v)
	simS<-sim[which(sim$Variable==v),];
	obsS<-obs[which(obs$Variable==as.character(v)),];

	#compute development time
	dev_rateS<-dev_rate[which(dev_rate$Variable==v),];
	P <- unlist(strsplit(as.character(dev_rateS$P), " "))
	Param <- strsplit(as.character(P), "=")
	for (j in 1:length(Param))
		assign(Param[[j]][1], as.double(Param[[j]][2]) )
	L <- lapply(Param, '[[', 1)	
	E <- parse(text = as.character(dev_rateS$Eq))
	
	T<-obsS$T
	obsS$sim_Time <- 1/pmin(1.2, pmax(0.001, eval(E)))
	obsS$sim_S <- obsS$s^obsS$sim_Time
	
	T <- seq(0,35,0.1)
	Rate <- pmin(1.2, pmax(0.001, eval(E)))
	MeanTime = 1/Rate
	LimitedMeanTime = MeanTime
	LimitedMeanTime[Rate<=0.01] = NA;
	
	Time_01 = LimitedMeanTime*qlnorm(0.01,-0.5*sigma^2,sigma)
	Time_99 = LimitedMeanTime*qlnorm(0.99,-0.5*sigma^2,sigma)
		
	rm(list=unlist(L))
	
	
	#compute survival
	P <- unlist(strsplit(as.character(simS$P), " "))
	Param <- strsplit(as.character(P), "=")
	for (j in 1:length(Param))
		assign(Param[[j]][1], as.double(Param[[j]][2]) )
	L <- lapply(Param, '[[', 1)	
	E<-parse(text = as.character(simS$Eq))

	s = pmin(1, pmax(0, eval(E)))
	S = s^MeanTime;
	
	
	rm(list=unlist(L))

	#compute survival deviation
	N=sum(obsS$n)
	lambda = S*N
	sigma = sqrt(lambda)
	Si_01 = pmax(0.001, qnorm(0.01, lambda, sigma))
	Si_99 = pmax(0.001, qnorm(0.99, lambda, sigma))
	
	S_01 = pmax(0.001, pmin(1, Si_01/N));
	S_99 = pmax(0.001, pmin(1, Si_99/N));
	s_01 = S_01^(1/LimitedMeanTime);
	s_99 = pmin(1,S_99^(1/LimitedMeanTime));
	
#1###############################################################################
	xLim = range(T)
	yLim = extendrange(range(obsS$s, na.rm=TRUE),f=0.8)
	yLim[2] = min(1,yLim[2])

	plot( NA, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim), labels = i==4 )
	axis(2, at = pretty(pmax(0,pmin(1,yLim))), labels = TRUE, las=TRUE)
	mtext(side=3, text=if(i==1)"a" else "", line=0.25, cex = par()$cex.lab);		
	
	
	polygon(c(T[!is.na(s_99)], rev(T[!is.na(s_01)])), c(s_99[!is.na(s_99)], rev(s_01[!is.na(s_01)])), col=gray(0.95), border=NA, lwd=1, lty = 1)
	cex = pmax(0.8, 1.8*log(obsS$n)/max(log(obsS$n)))
	lines( s~T, lwd=2.5, col=gray(0.5))
	points( obsS$s~obsS$T, pch=21, col = 'white', bg='black', lwd=1, cex=cex)
	
#2###############################################################################
	xLim = range(T)
	yLim = range(0, max(LimitedMeanTime, na.rm=TRUE))

	plot( NA, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim), labels = i==4)
	axis(2, at = pretty(yLim), las=TRUE)
	mtext(side=1, text=if(i==4)"Temperature (°C)" else "", line=2.5, cex = par()$cex.lab)
	mtext(side=3, text=if(i==1)"b" else "", line=0.25, cex = par()$cex.lab);		
	
	polygon(c(T[!is.na(Time_99)], rev(T[!is.na(Time_01)])), c(Time_99[!is.na(Time_99)], rev(Time_01[!is.na(Time_01)])), col=gray(0.95), border=NA, lwd=1, lty = 1)
	cex = pmax(0.8, 1.8*log(obsS$n)/max(log(obsS$n)))
	lines( MeanTime~T, lwd=2.5, col=gray(0.5))
	points( obsS$MeanTime~obsS$T, pch=21, col = 'white', bg='black', lwd=1, cex=cex)

#3###############################################################################
	
	
	obsS.lm <- lm(obsS$S~obsS$sim_S, weights=obsS$n)
	obsS.lm.R2 <- summary(obsS.lm)$r.squared 
	obsS.anova = anova(obsS.lm)
	
	
	xLim = range(T)
	yLim <- range(0,1.2)
	plot( NA, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
	axis(1, at = pretty(xLim), labels = i==4 )
	axis(2, at = pretty(range(0,1)), las=TRUE)
	mtext(side=3, text=if(i==1)"c" else "", line=0.25, cex = par()$cex.lab);		
	
	
	polygon(c(T[!is.na(S_99)], rev(T[!is.na(S_01)])), c(S_99[!is.na(S_99)], rev(S_01[!is.na(S_01)])), col=gray(0.95), border=NA, lwd=1, lty = 1)
	lines( S~T, lwd=2.5, col=gray(0.5))
	points( obsS$S~obsS$T, pch=21, col = 'white', bg='black', lwd=1, cex=cex)

	t1 = sprintf("n = %d",length(obsS$S[!is.na(obsS$S)]))
	t4 = sprintf("R² = %.3f",mean(obsS.lm.R2, na.rm=TRUE) )
	t5 = sprintf("F %d,%d = %.1f", obsS.anova[1,"Df"], obsS.anova[2, "Df"], obsS.anova[1, "F value"])
	t6 = ifelse(obsS.anova[1,"Pr(>F)"] < 0.001, "p.val < 0.001", sprintf("p.val = %.3f",  obsS.anova[1,"Pr(>F)"] ));

	legend('topleft',legend=c(t1,t4), inset = c(-0.1, -0.05), bty = "n", cex = par()$cex*0.8)
	legend('toprigh',legend=c(t5,t6), inset = c(0.0, -0.05), bty = "n", cex = par()$cex*0.8)
	
	
	mtext(side=4, text=v, line=0.5, cex = par()$cex.main, las=0)

################################################################################
}

mtext(side=2, text="Daily Survival", outer=TRUE, line=-1.25, cex = par()$cex.lab)
mtext(side=2, text="Development Time (days)", outer=TRUE, line=-15.5, cex = par()$cex.lab)
mtext(side=2, text="Overall Survival", outer=TRUE, line=-30.0, adj=c(0.5,0.5), padj=c(0.5,0.5), cex = par()$cex.lab)

dev.off()




