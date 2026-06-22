

# from sklearn.datasets import fetch_openml
# from sklearn.neural_network import MLPClassifier
# from sklearn.model_selection import train_test_split
# import numpy as np
# import emlearn
# import os

# #---------------------------------------------------------------------------------------------------------------------------
# results = []
# #---------------------------------------------------------------------------------------------------------------------------

# #---------------------------------------------------------------------------------------------------------------------------
# # downsample_14x14 :
# #       ---> inputs  : X : numpy array of shape (N, 784)
# #       ---> outputs : numpy array of shape (N, 196)
# #---------------------------------------------------------------------------------------------------------------------------
# def downsample_14x14(X):
#     imgs = X.reshape(-1, 28, 28)
#     return imgs.reshape(-1, 14, 2, 14, 2).mean(axis=(2, 4)).reshape(-1, 196)
# #---------------------------------------------------------------------------------------------------------------------------

# #---------------------------------------------------------------------------------------------------------------------------
# # load_mnist :
# #   ---> loads MNIST from sklearn, normalizes X to [0,1], splits train/test
# #   ---> outputs : X_train, X_test, y_train, y_test
# #---------------------------------------------------------------------------------------------------------------------------
# def load_mnist():
#     print("Loading MNIST...")
#     mnist = fetch_openml('mnist_784', version=1, as_frame=False)
#     X, y = mnist.data / 255.0, mnist.target.astype(int)
#     return train_test_split(X, y, test_size=0.2, random_state=42)
# #---------------------------------------------------------------------------------------------------------------------------

# #---------------------------------------------------------------------------------------------------------------------------
# # train_and_export :
# #   ---> trains an MLPClassifier and exports it as a C header via emlearn
# #   ---> inputs :
# #       ---> name          : label for this config (used in filename)
# #       ---> hidden        : tuple of hidden layer sizes e.g. (32,)
# #       ---> X_tr, X_te    : train and test feature arrays
# #       ---> y_tr, y_te    : train and test labels
# #   ---> outputs :
# #       ---> prints accuracy + header file size
# #       ---> saves .h file to disk
# #       ---> appends (name, accuracy, size_kb) to results[]
# #---------------------------------------------------------------------------------------------------------------------------
# def train_and_export(name, hidden, X_tr, X_te, y_tr, y_te):
#     print(f"\nTraining  ---> {name}")

#     # clf = MLPClassifier(hidden_layer_sizes=hidden, activation='relu',
#     #                     max_iter=50, random_state=42)

#     # clf = MLPClassifier(
#     #     hidden_layer_sizes=hidden,
#     #     activation='relu',
#     #     solver='sgd',
#     #     max_iter=50,
#     #     random_state=42
#     # )

#     # clf = MLPClassifier(
#     #     hidden_layer_sizes=(32,),
#     #     activation='relu',
#     #     solver='lbfgs',
#     #     max_iter=200,
#     #     random_state=42
#     # )

#     clf = MLPClassifier(
#         hidden_layer_sizes=(24,12),
#         activation='relu',
#         solver='lbfgs',
#         max_iter=500,
#         random_state=42
#     )

#     clf.fit(X_tr, y_tr)

#     acc     = clf.score(X_te, y_te)
#     fname   = f"mnist_{name}.h"

#     model = emlearn.convert(clf, method='inline')
#     model.save(file=fname, name='mnist_model')

#     size_kb = os.path.getsize(fname) / 1024

#     print(f"  ---> Accuracy    : {acc * 100:.2f}%")
#     print(f"  ---> Header size : {size_kb:.1f} KB")
#     print(f"  ---> Saved as    : {fname}")

#     results.append((name, acc * 100, size_kb))
# #---------------------------------------------------------------------------------------------------------------------------

# #---------------------------------------------------------------------------------------------------------------------------
# # print_summary :
# #---------------------------------------------------------------------------------------------------------------------------
# def print_summary():
#     print("\n" + "=" * 50)
#     print("  SUMMARY")
#     print("=" * 50)
#     print(f"  {'Config':<15} {'Accuracy':>10} {'Size (KB)':>12}")
#     print("-" * 50)
#     for name, acc, size in results:
#         print(f"  {name:<15} {acc:>9.2f}% {size:>10.1f} KB")
#     print("=" * 50)
# #---------------------------------------------------------------------------------------------------------------------------

# #---------------------------------------------------------------------------------------------------------------------------
# def __main__() -> None:

#     X_train, X_test, y_train, y_test = load_mnist()

#     X_train_14 = downsample_14x14(X_train)
#     X_test_14  = downsample_14x14(X_test)

#     # ---> 28x28 
#     train_and_export("784_32_10", (32,), X_train, X_test, y_train, y_test)
#     train_and_export("784_16_10", (16,), X_train, X_test, y_train, y_test)

#     # ---> 14x14 
#     train_and_export("196_32_10", (32,), X_train_14, X_test_14, y_train, y_test)

#     print_summary()

# __main__()
# #---------------------------------------------------------------------------------------------------------------------------






















from sklearn.datasets import fetch_openml
from sklearn.neural_network import MLPClassifier
from sklearn.model_selection import train_test_split
import numpy as np
# np.seterr(under='ignore')
np.seterr(all='ignore')
import emlearn
import os

#---------------------------------------------------------------------------------------------------------------------------
results = []
#---------------------------------------------------------------------------------------------------------------------------

#---------------------------------------------------------------------------------------------------------------------------
def downsample_14x14(X):
    imgs = X.reshape(-1, 28, 28)
    return imgs.reshape(-1, 14, 2, 14, 2).mean(axis=(2, 4)).reshape(-1, 196)
#---------------------------------------------------------------------------------------------------------------------------

#---------------------------------------------------------------------------------------------------------------------------
def load_mnist():
    print("Loading MNIST...")
    mnist = fetch_openml('mnist_784', version=1, as_frame=False)
    X, y = mnist.data / 255.0, mnist.target.astype(int)
    return train_test_split(X, y, test_size=0.2, random_state=42)
#---------------------------------------------------------------------------------------------------------------------------

#---------------------------------------------------------------------------------------------------------------------------
def train_and_export(name, hidden, X_tr, X_te, y_tr, y_te):
    print(f"\nTraining  ---> {name}  {hidden}")

    clf = MLPClassifier(
        hidden_layer_sizes=hidden,
        activation='relu',
        solver='sgd',
        learning_rate_init=0.01,
        max_iter=500,
        random_state=42
    )

    clf.fit(X_tr, y_tr)

    acc   = clf.score(X_te, y_te)
    fname = f"mnist_{name}.h"

    model = emlearn.convert(clf, method='inline')
    model.save(file=fname, name='mnist_model')

    size_kb = os.path.getsize(fname) / 1024

    print(f"  ---> Accuracy    : {acc * 100:.2f}%")
    print(f"  ---> Header size : {size_kb:.1f} KB")
    print(f"  ---> Saved as    : {fname}")

    results.append((name, acc * 100, size_kb))
#---------------------------------------------------------------------------------------------------------------------------

#---------------------------------------------------------------------------------------------------------------------------
def print_summary():
    print("\n" + "=" * 50)
    print("  SUMMARY")
    print("=" * 50)
    print(f"  {'Config':<20} {'Accuracy':>10} {'Size (KB)':>12}")
    print("-" * 50)
    for name, acc, size in results:
        print(f"  {name:<20} {acc:>9.2f}% {size:>10.1f} KB")
    print("=" * 50)
#---------------------------------------------------------------------------------------------------------------------------

#---------------------------------------------------------------------------------------------------------------------------
def __main__() -> None:

    X_train, X_test, y_train, y_test = load_mnist()

    X_train_14 = downsample_14x14(X_train)
    X_test_14  = downsample_14x14(X_test)

    # ---> 14x14 input only (196 features)
    train_and_export("196_32_10",    (32,),    X_train_14, X_test_14, y_train, y_test)
    train_and_export("196_24_12_10", (24, 12), X_train_14, X_test_14, y_train, y_test)

    print_summary()

__main__()
#---------------------------------------------------------------------------------------------------------------------------