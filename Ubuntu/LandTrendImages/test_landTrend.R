

E = read.csv("/home/remi/Project/LandTrendDemo/test_landTrend.csv")
str(E)

plot(y~x, E, pch=16)
lines(obs_yfit~x,E)
lines(obs_yfit~x,E)

