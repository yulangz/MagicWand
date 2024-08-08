import tensorflow as tf
import pandas as pd
import math


# 设置数据集的形状
timesteps = 200 # 2 秒，每秒 100 个数据点
lr = 4e-4
num_epochs = 50
batch_size = 8
input_dim = 3
num_classes = 9

# @tf.keras.utils.register_keras_serializable()
class Model(tf.keras.Model):
    def __init__(self):
        super(Model, self).__init__()
        self.conv1 = tf.keras.layers.Conv2D(filters=8, kernel_size=(1,3), strides=(1,1), padding='same')
        # self.norm1 = tf.keras.layers.BatchNormalization()
        self.relu1 = tf.keras.layers.Activation('relu')
        self.conv2 = tf.keras.layers.Conv2D(filters=16, kernel_size=(1,3), strides=(1,1), padding='same')
        # self.norm2 = tf.keras.layers.BatchNormalization()
        self.relu2 = tf.keras.layers.Activation('relu')
        # self.conv3 = tf.keras.layers.Conv2D(filters=32, kernel_size=(1,3), strides=(1,1), padding='same')
        # self.norm3 = tf.keras.layers.BatchNormalization()
        # self.relu3 = tf.keras.layers.Activation('relu')
        self.flatten = tf.keras.layers.Flatten()
        self.fc1 = tf.keras.layers.Dense(64, activation='relu')
        self.fc2 = tf.keras.layers.Dense(num_classes, activation='softmax')

    def call(self, x):
        x = tf.transpose(x, perm=[0, 2, 1])
        x = tf.expand_dims(x, axis=2)

        x = self.conv1(x)
        # x = self.norm1(x)
        x = self.relu1(x)

        x = self.conv2(x)
        # x = self.norm2(x)
        x = self.relu2(x)

        # x = self.conv3(x)
        # x = self.norm3(x)
        # x = self.relu3(x)

        x = self.flatten(x)
        x = self.fc1(x)
        x = self.fc2(x)

        return x

if __name__ == '__main__':

    train_x_pd = pd.read_csv('data_train_x.csv', header=None)
    train_y_pd = pd.read_csv('data_train_y.csv', header=None)
    test_x_pd = pd.read_csv('data_valid_x.csv', header=None)
    test_y_pd = pd.read_csv('data_valid_y.csv', header=None)

    train_x = tf.convert_to_tensor(train_x_pd.to_numpy(), dtype=tf.float32)
    train_x = tf.reshape(train_x, [-1, timesteps, input_dim])
    train_y = tf.convert_to_tensor(train_y_pd.to_numpy(), dtype=tf.int32)

    test_x = tf.convert_to_tensor(test_x_pd.to_numpy(), dtype=tf.float32)
    test_x = tf.reshape(test_x, [-1, timesteps, input_dim])
    test_y = tf.convert_to_tensor(test_y_pd.to_numpy(), dtype=tf.int32)

    train_data = tf.data.Dataset.from_tensor_slices((train_x, train_y))
    train_data = train_data.batch(batch_size)

    valid_data = tf.data.Dataset.from_tensor_slices((test_x, test_y))
    valid_data = valid_data.batch(batch_size)

    train_size = tf.data.experimental.cardinality(train_data).numpy()
    test_size = tf.data.experimental.cardinality(valid_data).numpy()
    steps_per_epoch = math.ceil(train_size / batch_size)
    validation_steps = math.ceil(test_size / batch_size)

    train_data = train_data.repeat()
    valid_data = valid_data.repeat()

    # 构建模型
    model = Model()

    # 编译模型
    model.compile(optimizer=tf.keras.optimizers.Adam(learning_rate=lr),
                loss=tf.keras.losses.SparseCategoricalCrossentropy(from_logits=False),
                metrics=['accuracy'])
    
    # 创建 checkpoint 对象
    checkpoint = tf.train.Checkpoint(optimizer=model.optimizer, model=model)
    manager = tf.train.CheckpointManager(checkpoint, './checkpoints', max_to_keep=3)

    callback = tf.keras.callbacks.ModelCheckpoint(filepath=manager.directory + "/ckpt_{epoch:02d}-{val_accuracy:.2f}-{val_loss:.2f}.keras",
                                              save_weights_only=False,
                                              monitor='val_accuracy',  # 监控验证损失
                                            #   save_best_only=True,
                                              save_freq='epoch',  # 每个 epoch 保存一次
                                              verbose=1)

    # 训练模型
    model.fit(train_x, train_y, epochs=num_epochs, shuffle=True, batch_size=batch_size, validation_data=(test_x, test_y), callbacks=[callback])

    loss, accuracy = model.evaluate(test_x, test_y)
    print(f"Validation loss: {loss}, validation accuracy: {accuracy}")

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()
    with open('model.tflite', 'wb') as f:
        f.write(tflite_model)