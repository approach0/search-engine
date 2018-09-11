import numpy as np
import matplotlib.pyplot as plt

# cat tmp | awk -v ORS="," '{print $2}' 
 
dat1 = (__dat1__)  
dat2 = (__dat2__)  

label1 = 'Baseline (CIKM results)' 
label2 = '__label__' 

# create plot
fig, ax = plt.subplots()
index = np.arange(len(dat1))
bar_width = 0.1
 
rects1 = plt.bar(index + 0 * bar_width, dat1, bar_width,
                 color='b',
                 label=label1)
 
rects2 = plt.bar(index + 1 * bar_width, dat2, bar_width,
                 color='r',
                 label=label2)
 
plt.xlabel('Queries (NTCIR-12)')
plt.ylabel('bpref')
plt.title('__label__')
x_sticks = tuple('query' + str(i) for i in range(1, 21) if i != 2) + tuple(['overall'])
plt.xticks(index + bar_width, x_sticks)
plt.legend()
 
w, h =(20, 8)
fig.set_size_inches(w, h)
# plt.show()

fig.savefig('./__prefix__/__label__.png')
plt.close(fig)    
