cat("\014")
rm(list=ls())
graphics.off()

# Needed only on Windows - run once per R session
# Adjust the path to match your installation of Ghostscript
Sys.setenv(R_GSCMD = "C:/Program Files/gs/gs9.53.1/bin/gswin64c.exe")

library(extrafont)
loadfonts(quiet=TRUE)
#loadfonts(device = "win")
#'JasmineUPC','Segoe Print','Constantia','Vijaya','Caladea','Gabriola',
myfont <- c('Andalus','MV Boli', 'Times New Roman')
mySize <- c(1.9,1.9,1.9)
font_sel = 3


sim <- read.csv("E:/Project/doc/CreateDevRatePDF.csv")
sim <- sim[order(sim$EqName),]
sim <- sim[!duplicated(sim$EqName),]

equations <- unique(sim$EqName)
Resolution = 600;






model_cex = rep(mySize[font_sel], length(equations))
model_cex[3] = model_cex[3]*1.4
model_cex[9] = model_cex[9]*0.75
model_cex[10] = model_cex[10]*0.65
model_cex[12] = model_cex[12]*0.8
model_cex[16] = model_cex[16]*0.9
model_cex[17] = model_cex[17]*0.8
model_cex[21] = model_cex[20]*1.1
model_cex[22] = model_cex[22]*0.6
model_cex[31] = model_cex[31]*1.3
model_cex[35] = model_cex[35]*0.9
model_cex[37] = model_cex[37]*1.2
model_cex[38] = model_cex[38]*1.0
model_cex[39] = model_cex[39]*1.0
model_cex[42] = model_cex[42]*1.0
model_cex[43] = model_cex[43]*1.3
model_cex[44] = model_cex[44]*0.9
model_cex[45] = model_cex[45]*0.65
model_cex[46] = model_cex[46]*0.9




file_name1 = paste("E:/Project/doc/DevRateEquations.pdf",sep="")
file_name2 = paste("E:/Project/doc/English/DevRateEquations.pdf",sep="")

pdf(file=file_name1, height=11, width=8.5, pointsize = 12, family = myfont[font_sel])
	par(mfrow=c(1,1), mar=c(0, 0, 0, 0), oma = c(2, 2, 2, 2), cex=1.0, cex.main = 0.7, cex.lab=1.1, cex.axis=1.0, family=myfont[font_sel])
	plot.new()
	text(0.5,1.0, adj=c(0.5,1.0), expression("BioSIM' Development Rate Models"), family = myfont[font_sel], cex = 2.0)
	text(0.5,.95, adj=c(0.5,0.95), expression("Standardized Parameters"), family = myfont[font_sel], cex = 1.8)
	text(0.5,0.89, adj=c(0.5,0.5), expression("Rémi Saint-Amant"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.86, adj=c(0.5,0.5), expression("2021"), family = myfont[font_sel], cex = 1.3)
	
	
	text(0.0,0.7, adj=c(0,0.5), expression("Scale factor"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.7, adj=c(0,0.5), expression(psi), family = myfont[font_sel], cex = 1.5)
	text(0.0,0.6, adj=c(0,0.5), expression("General Parameters"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.6, adj=c(0,0.5), expression(list(k,k[0],k[1],k[2],k[3],k[4])), family = myfont[font_sel], cex = 1.5)
	text(0.0,0.65, adj=c(0,0.5), expression("Sharpe&all Parameters"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.65, adj=c(0,0.5), expression(list(H[A],H[L],T[L],T[k[L]],H[H],T[H],T[k[H]])), family = myfont[font_sel], cex = 1.5)
	text(0.0,0.55, adj=c(0,0.5), expression("Temperature"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.55, adj=c(0,0.5), expression(T~'°C'~~~~bgroup('(','or'~T[k]~'in Kelvin',')')), family = myfont[font_sel], cex = 1.5)
	text(0.1,0.50, adj=c(0,0.5), expression("Lower"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.50, adj=c(0,0.5), expression(T[b]~'°C'), family = myfont[font_sel], cex = 1.5)
	text(0.1,0.45, adj=c(0,0.5), expression("Optimum"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.45, adj=c(0,0.5), expression(T[o]~'°C'), family = myfont[font_sel], cex = 1.5)
	text(0.1,0.40, adj=c(0,0.5), expression("Upper"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.40, adj=c(0,0.5), expression(T[m]~'°C'), family = myfont[font_sel], cex = 1.5)
	text(0.1,0.35, adj=c(0,0.5), expression("Others"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.35, adj=c(0,0.5), expression(T[omega]), family = myfont[font_sel], cex = 1.5)
	text(0.0,0.3, adj=c(0,0.5), expression("Temperature scale"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.3, adj=c(0,0.5), expression(list(Delta[T],~Delta[T[b]],~Delta[T[m]])), family = myfont[font_sel], cex = 1.5)
	text(0.0,0.25, adj=c(0,0.5), expression("Intermediate computation"), family = myfont[font_sel], cex = 1.5)
	text(0.5,0.25, adj=c(0,0.5), expression(list(beta,~beta[1],~beta[2],~Omega)), family = myfont[font_sel], cex = 1.5)
	

nbPlots=8
for( g in 1:nbPlots )
{
	


	par(mfrow=c(6,1), mar=c(0, 0, 0, 0), oma = c(2, 2, 2, 2), cex=1.0, cex.main = 0.7, cex.lab=1.1, cex.axis=1.0, family=myfont[font_sel])
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
		
		
		
		
		
		at_bottom = c("Deva&Higgis", "Hansen ( 2011 )", "Johnson_1974","Regniere_2012", "Wang&Engel_1998", "Wang&Lan&Ding_1982", "Lobry&Rosso&Flandrois_1993")
		adj_y = ifelse(e %in% at_bottom,0.8,0.55)
		
		if(e=="Saint-Amant_2021")e = 'Saint–Amant_2021'#strange problem with some font
			
		name = strsplit(e, "_")[[1]][1]
		year = strsplit(e, "_")[[1]][2]
		sep = "• ";
		if(is.na(year))year="" else year=paste(" (",year,")");
		
		
		
		plot.new()
		text(0.0,0.8, adj=c(0,0.8), sprintf("%02d%s%s%s", i, sep, name, year), family = myfont[font_sel], cex = 1.3)
		text(1,0.5, adj=c(1,adj_y), ex, family = myfont[font_sel], cex = model_cex[i])

		rm(list=unlist(L))
	}
}

par(mfrow=c(1,1), mar=c(0, 0, 0, 0), oma = c(2, 2, 2, 2), cex=1.0, cex.main = 0.7, cex.lab=1.1, cex.axis=1.0, family=myfont[font_sel])
	plot.new()
	
	text(0.0,1.0, adj=c(0.0,0.0), "Reference", family = myfont[font_sel], cex = 1.5)
	text(0.05,0.85, adj=c(0.0,0.0), "Sporleder M, Tonnang HEZ, Carhuapoma P, Gonzales JC, Juarez H, Kroschel J. 2013.\n    Insect Life Cycle Modeling (ILCYM) software a new tool for Regional and Global\n    Insect Pest Risk Assessments under Current and Future Climate Change Scenarios.\n    In: Peña JE, ed. Potential invasive pests of agricultural crops. Wallingford: CABI\n    https://doi.org/10.1079/9781845938291.0412", family = myfont[font_sel], cex = 1.)
	text(0.05,0.7, adj=c(0.0,0.0), "Rebaudo, F., Struelens, Q., Dangles, O. (2018).\n    Modelling temperature-dependent development rate and phenology in arthropods:\n    the DEVRATE package for R. Methods in Ecology & Evolution, 9(4), 1144-1150.\n    https://doi.org/10.1111/2041-210X.12935", family = myfont[font_sel], cex = 1.)


dev.off()

#As the name suggests, embed_fonts() will embed the fonts:
embed_fonts(file_name1, outfile=file_name2)





#ex <- expression(Alpha-Beta-Gamma-Delta-Zeta-Eta-Theta-Iota-Kappa-Lambda-Mu-Nu-Xi-Omicron-Pi-Rho-Tau-Upsilon-Phi-Chi-Psi-Omega)
#legend( "topleft", ex, bty = "n", cex = 1.2)
#ex <- expression(alpha-beta-gamma-delta-zeta-eta-theta-iota-kappa-lambda-mu-nu-xi-omicron-pi-rho-tau-upsilon-phi-chi-psi-omega)
#legend( "left", ex, bty = "n", cex = 1.2)
#, scriptstyle('for'~T%in%group('[',list(T[b],T[m]),']')~', 0 otherwise')



# f <- fonts()
# png(file=paste("G:/Travaux/Laricobiusosakensis/output/test_fonts.png",sep=''), height=length(f), width=5.5, units = "in", res = 300, pointsize = 9)
# par(mfrow=c(length(f),1), mar=c(0, 0, 0, 0), oma = c(0, 0, 0, 0), cex=1.0, cex.main = 0.7, cex.lab=1.1, cex.axis=1.0)
# {
	# e<-equations[9]
	# simS<-sim[which(sim$EqName==e),]
	# ex <- parse(text = simS$Math)
	
	# for(i in 1:length(f)) 
	# {
		#i=9
		
		
		
		
		# plot.new()
		# text(0.1,0.5, f[i], family = f[i], cex = 1.0)
		# text(1.0,0.5, adj=c(1,0.5), ex, family = f[i], cex = 1.0)
		

		
		# rm(list=unlist(L))
	# }
	
	# dev.off()
# }



e<-equations[37]
simS<-sim[which(sim$EqName==e),]
			
P <- unlist(strsplit(as.character(simS$P), " "))
Param <- strsplit(as.character(P), "=")
for (j in 1:length(Param))
	assign(Param[[j]][1], as.double(Param[[j]][2]) )
L <- lapply(Param, '[[', 1)	
E<-parse(text = as.character(simS$Eq))

"psi~e^~bgroup('[',scriptstyle(-k[1]~bgroup('(',T[omega]-T,')')^2)~+~bgroup('(',over(1,-k[2]~bgroup('(',T[m]-T,')')),')'),']')"
ex <- parse(text = simS$Math)