cat("\014")
rm(list=ls())
graphics.off()

library("extrafont")
# Needed only on Windows - run once per R session
# Adjust the path to match your installation of Ghostscript
Sys.setenv(R_GSCMD = "C:/Program Files/gs/gs10.02.1/bin/gswin64c.exe")





sim <- read.csv("E:/Project/doc/CreateDevRatePDF.csv")
sim <- sim[order(sim$EqName),]
sim <- sim[!duplicated(sim$EqName),]
sim <- sim[sim$EqName!="Regniere_1982b"&sim$EqName!="Regniere_1987b",]

equations <- unique(sim$EqName)
Resolution = 600;


mySize <- 1.9
model_cex = as.list(rep(mySize, length(equations)))
names(model_cex) = equations
#model_cex[[equations]] = mySize
#rep(mySize, length(equations))
model_cex[["Angilletta_2006"]] = 	mySize*1.4
model_cex[["Deva&Higgis"]] = 		mySize*0.65
model_cex[["Hansen_2011"]] = 		mySize*0.8
model_cex[["Hilbert&LoganIII"]] = 	mySize*0.9
model_cex[["Johnson_1974"]] = 		mySize*0.8
model_cex[["Kontodimas_2004"]] = 	mySize*1.1
model_cex[["Lactin2_1995"]] = 		mySize*1.1
model_cex[["Lobry&Rosso&Flandrois_1993"]] = mySize*0.6
model_cex[["Regniere_2012"]] = mySize*0.9
model_cex[["Saint-Amant_2022"]] = mySize*0.9
model_cex[["Schoolfield_1981"]] = mySize*1.0
model_cex[["Sharpe&DeMichele_1977"]] = mySize*1.0
model_cex[["Stinner_1974"]] = mySize*1.0
model_cex[["Taylor_1981"]] = mySize*1.3
model_cex[["Wagner_1988"]] = mySize*0.9
model_cex[["Wang&Engel_1998"]] = mySize*0.65
model_cex[["Wang&Lan&Ding_1982"]] = mySize*0.9

 

file_name = paste("E:/Project/doc/English/DevRateEquations.pdf",sep="")
#


pdf(file=file_name, height=11, width=8.5, pointsize = 12, family = "Times")
	par(mfrow=c(1,1), mar=c(0, 0, 0, 0), oma = c(2, 2, 2, 2), cex=1.0, cex.main = 0.7, cex.lab=1.1, cex.axis=1.0, family="Times")
	plot.new()
	text(0.5,1.0, adj=c(0.5,1.0), expression("BioSIM' Development Rate Models"), family = "Times", cex = 2.0)
	text(0.5,.95, adj=c(0.5,0.95), expression("Standardized Parameters"), family = "Times", cex = 1.8)
	text(0.5,0.89, adj=c(0.5,0.5), expression("Rémi Saint-Amant"), family = "Times", cex = 1.5)
	text(0.5,0.86, adj=c(0.5,0.5), expression("Version 1.0.1 (2022-03-01)"), family = "Times", cex = 1.3)
	
	
	text(0.0,0.7, adj=c(0,0.5), expression("Scale factor"), family = "Times", cex = 1.5)
	text(0.5,0.7, adj=c(0,0.5), expression(psi), family = "Times", cex = 1.5)
	text(0.0,0.65, adj=c(0,0.5), expression("Sharpe&all parameters"), family = "Times", cex = 1.5)
	text(0.5,0.65, adj=c(0,0.5), expression(list(H[A],H[L],T[L],T[k[L]],H[H],T[H],T[k[H]],T[k[o]])), family = "Times", cex = 1.5)
	text(0.0,0.60, adj=c(0,0.5), expression("Sharpe&all parameters scale factor"), family = "Times", cex = 1.5)
	text(0.5,0.60, adj=c(0,0.5), expression("F = 10 000"), family = "Times", cex = 1.5)
	text(0.0,0.55, adj=c(0,0.5), expression("General parameters"), family = "Times", cex = 1.5)
	text(0.5,0.55, adj=c(0,0.5), expression(list(k,k[0],k[1],k[2],k[3],k[4])), family = "Times", cex = 1.5)
	text(0.0,0.50, adj=c(0,0.5), expression("Temperature"), family = "Times", cex = 1.5)
	text(0.5,0.50, adj=c(0,0.5), expression(T~'°C'~~~~bgroup('(','or'~T[k]~'in Kelvin',')')), family = "Times", cex = 1.5)
	text(0.1,0.45, adj=c(0,0.5), expression("Lower"), family = "Times", cex = 1.5)
	text(0.5,0.45, adj=c(0,0.5), expression(T[b]~'°C'), family = "Times", cex = 1.5)
	text(0.1,0.40, adj=c(0,0.5), expression("Optimum"), family = "Times", cex = 1.5)
	text(0.5,0.40, adj=c(0,0.5), expression(T[o]~'°C'), family = "Times", cex = 1.5)
	text(0.1,0.35, adj=c(0,0.5), expression("Upper"), family = "Times", cex = 1.5)
	text(0.5,0.35, adj=c(0,0.5), expression(T[m]~'°C'), family = "Times", cex = 1.5)
	text(0.1,0.30, adj=c(0,0.5), expression("Others"), family = "Times", cex = 1.5)
	text(0.5,0.30, adj=c(0,0.5), expression(T[omega]), family = "Times", cex = 1.5)
	text(0.0,0.25, adj=c(0,0.5), expression("Temperature scale"), family = "Times", cex = 1.5)
	text(0.5,0.25, adj=c(0,0.5), expression(list(Delta[T],~Delta[T[b]],~Delta[T[m]])), family = "Times", cex = 1.5)
	text(0.0,0.20, adj=c(0,0.5), expression("Intermediate computation"), family = "Times", cex = 1.5)
	text(0.5,0.20, adj=c(0,0.5), expression(list(beta,~beta[1],~beta[2],~Omega)), family = "Times", cex = 1.5)
	

nbPlots=8
for( g in 1:nbPlots )
{
	


	par(mfrow=c(6,1), mar=c(0, 0, 0, 0), oma = c(2, 2, 2, 2), cex=1.0, cex.main = 0.7, cex.lab=1.1, cex.axis=1.0, family="Times")
	for(i in (6*(g-1)+1):min(length(equations),6*g) )
	{
		#i=9
		e<-equations[i]
		simS<-sim[which(sim$EqName==e),]
					
		P <- unlist(strsplit(as.character(simS$P), " "))
		Param <- strsplit(as.character(P), "=")
		for (j in 1:length(Param))
			assign(Param[[j]][1], as.double(Param[[j]][2]) )
		L <- lapply(Param, '[[', 1)	
		E<-parse(text = as.character(simS$Eq))
		
		
		ex <- parse(text = simS$Math)
		
		at_bottom = c("Deva&Higgis", "Hansen_2011", "Johnson_1974","Regniere_2012", "Wang&Engel_1998", "Wang&Lan&Ding_1982", "Lobry&Rosso&Flandrois_1993")
		adj_y = ifelse(e %in% at_bottom,0.8,0.55)
		
		#if(e=="Saint-Amant_2021")e = 'Saint–Amant_2021'#strange problem with some font
			
		name = strsplit(e, "_")[[1]][1]
		year = strsplit(e, "_")[[1]][2]
		sep = "• ";
		if(is.na(year))year="" else year=paste(" (",year,")");
		
		
		
		plot.new()
		text(0.0,0.8, adj=c(0,0.8), sprintf("%s%s%s", sep, name, year), family = "Times", cex = 1.3)
		text(1,0.5, adj=c(1,adj_y), ex, family = "Times", cex = model_cex[[e]])

		rm(list=unlist(L))
	}
}

par(mfrow=c(1,1), mar=c(0, 0, 0, 0), oma = c(2, 2, 2, 2), cex=1.0, cex.main = 0.7, cex.lab=1.1, cex.axis=1.0, family="Times")
	plot.new()
	
	text(0.0,1.0, adj=c(0.0,0.0), "Reference", family = "Times", cex = 1.5)
	text(0.05,0.85, adj=c(0.0,0.0), "Sporleder M, Tonnang HEZ, Carhuapoma P, Gonzales JC, Juarez H, Kroschel J. 2013.\n    Insect Life Cycle Modeling (ILCYM) software a new tool for Regional and Global\n    Insect Pest Risk Assessments under Current and Future Climate Change Scenarios.\n    In: Peña JE, ed. Potential invasive pests of agricultural crops. Wallingford: CABI\n    https://doi.org/10.1079/9781845938291.0412", family = "Times", cex = 1.)
	text(0.05,0.7, adj=c(0.0,0.0), "Rebaudo, F., Struelens, Q., Dangles, O. (2018).\n    Modelling temperature-dependent development rate and phenology in arthropods:\n    the DEVRATE package for R. Methods in Ecology & Evolution, 9(4), 1144-1150.\n    https://doi.org/10.1111/2041-210X.12935", family = "Times", cex = 1.)


dev.off()





#As the name suggests, embed_fonts() will embed the fonts:
embed_fonts(file_name, outfile=file_name);


