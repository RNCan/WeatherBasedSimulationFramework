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
         GetBioSIMOutputFilePath <- GetFilePath("output/DevTime.csv")
    } else { GetBioSIMOutputFilePath <- argv[grep("--args", argv)+1] 
    }
}


dir.create(GetFilePath("Images/"), showWarnings = FALSE);

obs_file_path = GetFilePath("Input/DevTime.csv");
obs = read.csv(obs_file_path);


if(!"Rate" %in% colnames(obs) & "Time" %in% colnames(obs))
	obs$Rate = 1/obs$Time;

sim_file_path = GetBioSIMOutputFilePath();
sim = read.csv(sim_file_path)


if("AICc" %in% colnames(sim)){
	#data need to be ordered
	sim = sim[ order(sim$AICc),]
	sim$pos = ave(sim$AICc, sim$Variable, FUN=function(x){order(x)})

	#keep only the best for each Variable+model
	sim = sim[order(sim$pos),]
	sim$VarEqName = paste(sim$Variable,sim$EqName)
	sim = sim[!duplicated(sim$VarEqName),]
	
	#recompute new order
	sim$pos = ave(sim$AICc, sim$Variable, FUN=function(x){order(x)})
	
	#get best model variables+equations over all Tb and Tm variations
	best_model = aggregate(AICc~Variable+EqName,data=sim,FUN=mean)
	best_model = sim[order(sim$Variable, sim$pos), c("Variable","EqName", "maxLL", "AICc", "pos")]
	
}else{
	#data need to be ordered
	sim = sim[order(-sim$R2),]
	sim$pos = ave(sim$R2, sim$Variable, FUN=function(x){order(-x)})
	
	#get best model variables+equations over all Tb and Tm variations
	best_model = aggregate(R2~Variable+EqName,data=sim,FUN=mean)
	best_model = best_model[order(-best_model$R2),]
}

print(best_model)

#compute best model over all variables
overall_best = aggregate(AICc~EqName, best_model[best_model$Variable!="Adult",], FUN=mean)
overall_best = overall_best[order(overall_best$AICc),]
print(overall_best)



#order by AICC
sim = sim[order(sim$Variable,sim$pos),]
variables = unique(sim$Variable)

RATE_MAX = aggregate(Rate~Variable, obs, FUN=max)$Rate

for( vv in 1:length(variables) )
{
#vv=2
	v=variables[vv]
	print(v)
	
	simS=sim[which(sim$Variable==v),];
	obsS=obs[which(obs$Variable==as.character(v)),];
	
	equations = unique(simS$EqName)
	
	NB_IMAGES_PER_PLOT = 15;
	NB_COLS = 5;
	if(length(equations)<=15){
		NB_IMAGES_PER_PLOT = 15;
	}else if(length(equations)<=20){
		NB_IMAGES_PER_PLOT = 20;
	}else if(length(equations)<=25){
		NB_IMAGES_PER_PLOT = 25;
	}else
	{
		NB_COLS = 8;
		if(length(equations)<=32){
		NB_IMAGES_PER_PLOT = 32;
		}else if(length(equations)<=40){
			NB_IMAGES_PER_PLOT = 40;
		}else {
			NB_IMAGES_PER_PLOT = 48;
		}
	}
	
	NB_ROWS = NB_IMAGES_PER_PLOT/NB_COLS

	T = seq(0,35,0.5)
	
	
	#compute RATE_MAX
	rate = data.frame();
	param = list();
	
	for(i in 1:length(equations) )
	{
		#i=2
		simSE=simS[which(simS$EqName==equations[i]),]
		
		
		P = unlist(strsplit(as.character(simSE$P), " "))
		Param = strsplit(as.character(P), "=")
		names(Param) = unlist(lapply(Param, `[[`, 1))
		Param = data.frame(lapply(lapply(Param, function(x) {x[2]}), as.numeric))
		
		for (j in 1:ncol(Param))
			assign(names(Param)[j], Param[1,j] )

		E=parse(text = as.character(simSE$Eq));
		e=pmin(RATE_MAX[vv]*2, pmax(0, eval(E)));
		e_01 = e*qlnorm(0.01,-0.5*sigma^2,sigma);
		e_99 = e*qlnorm(0.99,-0.5*sigma^2,sigma);
		rate = rbind( rate, data.frame(Equation=equations[i],T,e,e_01,e_99));
		param[[i]] = list( sim=simSE, P=Param);
		
		rm(list=names(Param))
	}

	
	
	
	xLim = range(T)
	yLim = range(0,max(rate$e_99, na.rm=TRUE))
	
	#create images with plot by page
	nbPlots = ceiling(length(equations)/NB_IMAGES_PER_PLOT)
	
	for( g in 1:nbPlots )
	{
		
		file_title = unlist(strsplit(basename(sim_file_path), "\\."))[1]
		
		png(file=GetFilePath(paste("Images/",file_title,v,".png",sep='')), height=8.5, width=14, units = "in", res = Resolution, pointsize = 10)
		par(mfrow=c(NB_ROWS,NB_COLS), mar=c(0.6, 0.6, 0.4, 0.4), oma = c(3.5, 4.0, 2.5, 0.3), font.main=1, cex=1.0, cex.main = 1.3, cex.lab=1.1, cex.axis=1.0)
		for(i in (NB_IMAGES_PER_PLOT*(g-1)+1):min(length(equations),NB_IMAGES_PER_PLOT*g) )
		{
			#i=1
			r = rate[rate$Equation==equations[i],]
			P = param[[i]]$P
			simSE=param[[i]]$sim
			
			
			plot( NA, las=TRUE, xlim=xLim, ylim=yLim, main="", xlab="", ylab="", xaxt = "n", yaxt = "n" , frame=FALSE )
			axis(1, at = pretty(xLim), labels = ((i-1)%%NB_IMAGES_PER_PLOT)>=NB_IMAGES_PER_PLOT|(i>length(equations)-NB_COLS) )
			axis(2, at = pretty(yLim), labels = ((i-1)%%NB_COLS)==0, las=TRUE)

			
			cex = 0.8#pmax(0.8, 1.8*log(obsS$n)/max(log(obsS$n)))
			#polygon(c(T, rev(T)), c(r$e_99, rev(r$e_01)), col=gray(0.9), border=NA, lwd=1, lty = 1)
			polygon(c(T[!is.na(r$e_99)], rev(T[!is.na(r$e_01)])), c(r$e_99[!is.na(r$e_99)], rev(r$e_01[!is.na(r$e_01)])), col=gray(0.95), border=NA, lwd=1, lty = 1)
			lines( e~T, r, lwd=2)
			points( obsS$Rate~obsS$T, pch=21, col = 'white', bg='black', lwd=.8, cex=cex)

			if("R2" %in% colnames(simSE)){
				t1 = sprintf("R² = %.3f", simSE$R2 )
				t2 = ""
			}else {
				t1 = sprintf("maxLL = %.1f",simSE$maxLL)
				t2 = sprintf("AICc = %.1f", simSE$AICc )
			}
			
			name = strsplit(simSE$EqName, "_")[[1]][1]
			year = strsplit(simSE$EqName, "_")[[1]][2]
			if(is.na(year))year="" else year=paste("(",year,")",sep="");
			
			legend('topleft',legend=c(as.character(paste(name,year,sep=""))), bty = "n", cex = par()$cex*0.9) 
			legend('topleft',legend=c(t1,t2), inset = c(0.08, 0.14), bty = "n", cex = par()$cex*0.75) 
			legend('topright',legend=simSE$pos, text.col='blue', bty = "n", cex = par()$cex*0.8) 
			
			
			if(!is.na(P$Tb))
				text(max(6, P$Tb+4), 0, labels=round(P$Tb,1), col='blue', cex = par()$cex*0.9)
			if(!is.na(P$Tm))
				text(P$Tm-3, 0, labels=round(P$Tm,1), col='red', cex = par()$cex*0.9)

		}
		
		
		mtext("Temperature (°C)", side=1, outer = TRUE, line=2.0, cex = par()$cex.lab)
		mtext("Development Rate (1/d)", side=2, outer = TRUE, line=2.5, cex = par()$cex.lab)
		mtext(bquote("Development Rate for "*italic(Insect.~name)*": "*.(v)), outer = TRUE, line=0.2, cex = par()$cex.main)

		# Turn off device driver (to flush output to PDF)
		dev.off()
	}
}




#save best equations for each variables
best_sim = sim[sim$pos==1,]
print(best_sim[, c("Variable","EqName")])

write.csv(best_sim, GetFilePath("Output/DevTime(Best).csv"),quote = TRUE, row.names = FALSE)




