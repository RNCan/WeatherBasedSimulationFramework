cat("\014")
rm(list=ls())
graphics.off()
library(stringi)

# Needed only on Windows - run once per R session
# Adjust the path to match your installation of Ghostscript
Sys.setenv(R_GSCMD = "C:/Program Files/gs/gs9.53.1/bin/gswin64c.exe")

library(extrafont)
loadfonts(quiet = TRUE)

myfont <- c('Andalus','MV Boli', 'Times New Roman')
mySize <- c(1.9,1.9,1.9)
font_sel = 1


sim <- read.csv("E:/Project/doc/CreateSurvivalPDF.csv")
sim <- sim[!duplicated(sim$EqName),]
pos <- !stri_detect_fixed(sim$EqName, "Survival_")
sim <- sim[order(pos, sim$EqName),]

equations <- unique(sim$EqName)
Resolution = 600;






 model_cex = rep(mySize[font_sel], length(equations))


file_name = paste("E:/Project/doc/SurvivalEquations.pdf",sep="")

pdf(file=file_name, height=11, width=8.5, pointsize = 12, family = myfont[font_sel])

	plot.new()
	text(0.5,1.0, adj=c(0.5,0.9), expression("BioSIM' Survival Models"), family = myfont[font_sel], cex = 2.0)
	text(0.5,.92, adj=c(0.5,0.9), expression("Standardized Parameters"), family = myfont[font_sel], cex = 1.8)
	text(0.0,0.7, adj=c(0,0.5), expression("General Parameters"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.7, adj=c(0,0.5), expression(list(k,k[0],k[1],k[2],kk, kk[1],kk[2])), family = myfont[font_sel], cex = 1.5)
	text(0.0,0.6, adj=c(0,0.5), expression("Temperature"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.6, adj=c(0,0.5), expression(T~'°C'), family = myfont[font_sel], cex = 1.5)
	text(0.1,0.55, adj=c(0,0.5), expression("Lower"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.55, adj=c(0,0.5), expression(T[L]~'°C'), family = myfont[font_sel], cex = 1.5)
	text(0.1,0.5, adj=c(0,0.5), expression("Optimum"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.5, adj=c(0,0.5), expression(T[o]~'°C'), family = myfont[font_sel], cex = 1.5)
	text(0.1,0.45, adj=c(0,0.5), expression("Upper"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.45, adj=c(0,0.5), expression(T[H]~'°C'), family = myfont[font_sel], cex = 1.5)
	text(0.0,0.3, adj=c(0,0.5), expression("Temperature scale"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.3, adj=c(0,0.5), expression(list(Delta[T],~Delta[T[L]],~Delta[T[H]])), family = myfont[font_sel], cex = 1.5)


nbPlots=3
for( g in 1:nbPlots )
{
	

	#g=1
	par(mfrow=c(6,1), mar=c(0, 0, 0, 0), oma = c(2, 2, 2, 2), cex=1.0, cex.main = 0.7, cex.lab=1.1, cex.axis=1.0, family=myfont[font_sel])
	for(i in (6*(g-1)+1):min(length(equations),6*g) )
	{
	print(i)
		#i=1
		e<-equations[i]
		simS<-sim[which(sim$EqName==e),]
					
		P <- unlist(strsplit(as.character(simS$P), " "))
		Param <- strsplit(as.character(P), "=")
		for (j in 1:length(Param))
			assign(Param[[j]][1], as.double(Param[[j]][2]) )
		L <- lapply(Param, '[[', 1)	
		E<-parse(text = as.character(simS$Eq))
		
		
		ex <- parse(text = simS$Math)
		
		
		
		
		
		at_bottom = c("Deva&Higgis", "Hansen ( 2011 )", "Johnson_1974","Regniere_2012", "Wang&Engel_1998", "Wang&Lan&Ding_1982", "Lobry&Rosso&Flandrois_1993")
		adj_y = ifelse(e %in% at_bottom,0.8,0.55)
		
		if(e=="Saint-Amant_2019")e = 'Saint–Amant_2019'#strange problem with some font
			
		sep = "• ";
		
		
		plot.new()
		text(0.0,0.8, adj=c(0,0.8), sprintf("%02d%s%s", i, sep, e), family = myfont[font_sel], cex = 1.3)
		text(1,0.5, adj=c(1,adj_y), ex, family = myfont[font_sel], cex = model_cex[i])
		rm(list=unlist(L))
	}
}
dev.off()

#As the name suggests, embed_fonts() will embed the fonts:
embed_fonts(file_name, outfile=paste("E:/Project/doc/SurvivalEquations.pdf",sep=""))



