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
		 GetBioSIMOutputFilePath <- GetFilePath("output/Survival.csv")
    } else { GetBioSIMOutputFilePath <- argv[grep("--args", argv)+1] 
    }
}

dir.create(GetFilePath("Images/"), showWarnings = FALSE)


obs_file_path = GetFilePath("Input/Survival.csv");
obs = read.csv(obs_file_path, na.string="NA");


#overall survival
if(!"S" %in% colnames(obs))
	obs$S = obs$Survival/obs$n;

#daily survival
if(!"s" %in% colnames(obs))
	obs$s = obs$S^(1/obs$MeanTime);


sim_file_path = GetFilePath("Output/Survival.csv")
sim = read.csv(sim_file_path)

if("AICc" %in% colnames(sim)){

	
	sim = sim[ order(sim$AICc),]
	sim$pos = ave(sim$AICc, sim$Variable, FUN=function(x){order(x)})
	best_model = sim[order(sim$Variable,sim$AICc), c("Variable","EqName", "maxLL", "AICc", "pos")];
	
}else{
	
	
	sim$pos = ave(sim$R2, sim$Variable, FUN=function(x){order(-x)})
	best_model <- aggregate(R2~Variable+EqName,data=sim,FUN=mean)
	best_model <- best_model[order(-best_model$R2),]
}

print(best_model)


variables = sort(unique(sim$Variable))

MAX_S = c(1,1,1.02,1)

for( vv in 1:length(variables) )
{
	
	v <- variables[vv]
	print(v)

	
	simS<-sim[which(sim$Variable==v),];
	obsS<-obs[which(obs$Variable==as.character(v)),];
	
	equations <- unique(simS$EqName)


	T <- seq(0,35,0.5)
	xLim = range(T)
	yLim<-range(MAX_S[vv],extendrange(range(obsS$s, na.rm=TRUE), f=0.6))
	
	file_title = unlist(strsplit(basename(sim_file_path), "\\."))[1]
	png(file=GetFilePath(paste("Images/",file_title,"_",v,".png",sep='')), height=8.5, width=8.5, units = "in", res = Resolution, pointsize = 11)
	par(mfrow=c(4,4), mar=c(1.0,0.5, 1.0, 0.5), oma = c(3.5, 4.5, 3.5, 0.5), cex=1.0, cex.main = 0.9, cex.lab=1.1, cex.axis=1.0)
	{
		for(i in 1:length(equations))
		{
			#i=1
			e<-equations[i]
			simSE<-simS[which(simS$EqName==e),]
						
			P <- unlist(strsplit(as.character(simSE$P), " "))
			Param <- strsplit(as.character(P), "=")
			for (j in 1:length(Param))
				assign(Param[[j]][1], as.double(Param[[j]][2]) )
			L <- lapply(Param, '[[', 1)	
			E<-parse(text = as.character(simSE$Eq))
			
			
			s<-pmin(1, pmax(0, eval(E)))
			
			
			plot( NA, xlim=xLim, ylim=yLim, main=as.character(simSE$EqName), xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
			axis(1, at = pretty(xLim), labels = (i%%20)>16|(i>length(equations)-4) )
			axis(2, at = pretty(pmin(1,yLim)), labels = !((i-1)%%4), las=TRUE)
			
			cex = pmax(0.8, 1.8*log(obsS$n/2)/log(max(obsS$n)))
			lines( s~T, lwd=2.5, col=gray(0.5))
			points( obsS$s~obsS$T, pch=21, col = 'white', bg='black', lwd=0.8, cex=cex)

			if(simSE$maxLL<10000)
			{
				#if("R2" %in% colnames(simSE)){
					t0 = sprintf("R² = %.3f", simSE$R2 )
				#	t2 = ""
				#}else {
					t1 = sprintf("maxLL = %.1f",simSE$maxLL)
					t2 = sprintf("AICc = %.1f", simSE$AICc )
				#}
				
				legend('topleft',legend=c(t0,t1,t2), bty = "n", cex = par()$cex*0.8) 
				legend('topright',legend=simSE$pos, text.col='blue', bty = "n", cex = par()$cex*0.8) 
			}
#, inset = c(0.08, 0.14)

			legend_text <- character()
			for (j in 1:length(Param))
			{
				if(Param[[j]][1] != "sigma")
					legend_text[j] = sprintf("%s=%.3f", Param[[j]][1], as.double(Param[[j]][2]) )
			}
			
			#legend('topright',legend=legend_text, bty = "n", cex = par()$cex*0.8) 

################################################################################

			rm(list=unlist(L))
		}
		
		
		mtext("Temperature (°C)", side=1, outer = TRUE, line=2.0, cex = par()$cex.lab*1.2)
		mtext("Daily Survival", side=2, outer = TRUE, line=3.0, cex = par()$cex.lab*1.2)
		mtext(bquote("Survival for "*italic(Insect.~name)*": "*.(v)), outer = TRUE, line=0.7, cex = par()$cex.main*1.9)
		
		dev.off()
	}

}



#Select and save best 
BestEq = c("Survival_15", "Survival_02","Survival_08","Survival_13");

best=data.frame();
for(i in 1:length(variables))
	best = rbind( best, sim[sim$Variable==variables[i]&sim$EqName==BestEq[i],])

write.csv(best, GetFilePath("Output/Survival(Best).csv"),quote = TRUE, row.names = FALSE)


