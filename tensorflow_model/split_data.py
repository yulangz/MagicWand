import pandas as pd
from sklearn.model_selection import train_test_split
# 读取数据
data_x = pd.read_csv('data_x.csv', header=None)
data_y = pd.read_csv('data_y.csv', header=None)
data_x = pd.DataFrame(data_x.values.tolist())
data = pd.concat([data_x, data_y], axis=1)
data = data[data.iloc[:, -1].isin([0,1,2,3,5])]
data.iloc[:, -1] = data.iloc[:, -1].replace(5, 4)
print(data.shape)
data_x = data.iloc[:,:-1]
data_y = data.iloc[:,-1]
# 切分数据
X_train, X_test, y_train, y_test = train_test_split(data_x, data_y, test_size=0.2, random_state=42)
# 写入到文件
X_train.to_csv('data_train_x.csv', index=False, header=False)
y_train.to_csv('data_train_y.csv', index=False, header=False)
X_test.to_csv('data_valid_x.csv', index=False, header=False)
y_test.to_csv('data_valid_y.csv', index=False, header=False)
