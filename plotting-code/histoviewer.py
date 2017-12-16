import numpy as np
import matplotlib.pyplot as plt
import matplotlib.cm as cm
kkk=cm.gist_stern # Put the Plotting Style here
temp = np.loadtxt('quark_1e5_13tev-eflow.txt')
temp2 = np.loadtxt('gluon_1e5_13tev-eflow.txt')
#temp = np.loadtxt('quark_1e5_13tev.txt')
#temp2 = np.loadtxt('gluon_1e5_13tev.txt')
Titles = ['E2','SubJettiness','Girth']
#save = ['e2.png','tau.png','girth.png']
save = ['e2-eflow.png','tau-eflow.png','girth-eflow.png']
for i in range(len(temp[1,:])):
  t = max(temp[:,i])
  t2 = max(temp2[:,i])
  plt.title(Titles[i])
  plt.hist(temp[:,i], 50, normed=1, histtype='step',label ='Quark')
  plt.hist(temp2[:,i], 50, normed=1, histtype='step',label = 'Gluon')
  plt.legend(loc='upper right')
  plt.xlim([0,max(t,t2)*1.1])
  plt.savefig(save[i])
  plt.close()