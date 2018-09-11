import numpy as np
import matplotlib.pyplot as plt

# cat tmp | awk -v ORS="," '{print $2}' 
 
dat1 = (0.7391,0.2441,0.4365,0.9714,0.8611,0.3509,0.1890,0.5988,0.5720,1.0000,0.5417,0.3980,0.5682,0.2154,0.4537,0.7057,0.2652,0.5936,0.6190)  
dat2 = (0.9565,0.3980,0.5397,0.9714,0.6389,0.2456,0.2625,0.6358,0.5871,0.9000,0.5417,0.3673,0.4855,0.2196,0.4213,0.6467,0.2983,0.4602,0.6639)  

label1 = 'submitted CIKM original multi-subtree' 
label2 = 'Naive node coverage scoring formula' 

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
plt.title('Precise normalized node coverage accuracy results [partial relevance]')
plt.xticks(index + bar_width, tuple('query' + str(i) for i in range(1, 21) if i != 2))
plt.legend()
 
plt.tight_layout()
plt.show()
