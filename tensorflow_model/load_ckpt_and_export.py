import tensorflow as tf
import pandas as pd
import math

# 设置数据集的形状
timesteps = 200 # 2 秒，每秒 100 个数据点
lr = 1e-4
num_epochs = 200
batch_size = 8
input_dim = 3
num_classes = 9

@tf.keras.utils.register_keras_serializable()
class Model(tf.keras.Model):
    def __init__(self, trainable, dtype):
        super(Model, self).__init__()
        self.conv1 = tf.keras.layers.Conv2D(filters=32, kernel_size=(1,3), strides=(1,1), padding='same')
        self.norm1 = tf.keras.layers.BatchNormalization()
        self.relu1 = tf.keras.layers.Activation('relu')
        self.conv2 = tf.keras.layers.Conv2D(filters=64, kernel_size=(1,3), strides=(1,1), padding='same')
        self.norm2 = tf.keras.layers.BatchNormalization()
        self.relu2 = tf.keras.layers.Activation('relu')
        self.conv3 = tf.keras.layers.Conv2D(filters=128, kernel_size=(1,3), strides=(1,1), padding='same')
        self.norm3 = tf.keras.layers.BatchNormalization()
        self.relu3 = tf.keras.layers.Activation('relu')
        self.flatten = tf.keras.layers.Flatten()
        self.fc1 = tf.keras.layers.Dense(64, activation='relu')
        self.fc2 = tf.keras.layers.Dense(num_classes, activation='softmax')

    def call(self, x):
        x = tf.transpose(x, perm=[0, 2, 1])
        x = tf.expand_dims(x, axis=2)

        x = self.conv1(x)
        x = self.norm1(x)
        x = self.relu1(x)

        x = self.conv2(x)
        x = self.norm2(x)
        x = self.relu2(x)

        x = self.conv3(x)
        x = self.norm3(x)
        x = self.relu3(x)

        x = self.flatten(x)
        x = self.fc1(x)
        x = self.fc2(x)

        return x


if __name__ == '__main__':
    test_x_pd = pd.read_csv('data_valid_x.csv', header=None)
    test_y_pd = pd.read_csv('data_valid_y.csv', header=None)

    test_x = tf.convert_to_tensor(test_x_pd.to_numpy(), dtype=tf.float32)
    test_x = tf.reshape(test_x, [-1, timesteps, input_dim])  # 确保 timesteps 和 input_dim 已定义
    test_y = tf.convert_to_tensor(test_y_pd.to_numpy(), dtype=tf.int32)

    valid_data = tf.data.Dataset.from_tensor_slices((test_x, test_y))
    valid_data = valid_data.batch(batch_size)  # 确保 batch_size 已定义

    test_size = tf.data.experimental.cardinality(valid_data).numpy()
    validation_steps = math.ceil(test_size / batch_size)

    model = tf.keras.models.load_model('./checkpoints/ckpt_47-0.96-0.61.keras', custom_objects={'Model': Model})

    loss, accuracy = model.evaluate(valid_data)
    print(f"Validation loss: {loss}, validation accuracy: {accuracy}")

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()
    with open('model.tflite', 'wb') as f:
        f.write(tflite_model)