
from numpy import *

from topicmod.external.moremath import *

from topicmod.util import flags

flags.define_string("alpha", None, "The current value of alpha")
flags.define_string("gamma", None, "The current gamma matrix")
flags.define_float("tolerance", 0.001, "Toleranceg for convergence")

NEGATIVE_INFINITY = -float("inf")

def l_alpha(alpha, M, K, gamma_grad):
  val = lngamma(sum(alpha)) - sum(lngamma(x) for x in alpha)
  val *= M
  for ii in xrange(K):
    val += alpha[ii] * gamma_grad[ii]
  return val

def compute_gamma_gradient(gamma, K):
  """
  Compute the components of the derivative that gamma contributes to. 
  """

  grad = zeros(K)

  for gamma_d in gamma:
    digam_gamma_sum = digamma(sum(gamma_d))
    for ii in xrange(K):
      grad[ii] += digamma(gamma_d[ii]) - digam_gamma_sum
  return grad
      

if __name__ == "__main__":
  flags.InitFlags()
  tau = 0.1

  new_alpha = genfromtxt(flags.alpha)
  gamma = genfromtxt(flags.gamma)
  
  K = len(new_alpha)
  M = len(gamma)

  gamma_grad = compute_gamma_gradient(gamma, K)
  old_alpha = zeros(K) - 1.0

  print sum(abs(old_alpha - new_alpha))

  elbo = NEGATIVE_INFINITY
  failure = False
  while(sum(abs(old_alpha - new_alpha)) > flags.tolerance or failure):
    old_alpha = new_alpha.copy()

    # Compute gradient
    grad = gamma_grad.copy()

    digamma_alpha_sum = digamma(sum(old_alpha))
    for ii in xrange(K):
      grad[ii] += M * (digamma_alpha_sum - digamma(old_alpha[ii]))

    # Compute diagonal of Hessian
    h = zeros(K)
    for ii in xrange(K):
      h[ii] = M*trigamma(old_alpha[ii])

    # Compute off-diagonal Hessian term
    z = -M * trigamma(sum(old_alpha))

    c = sum(grad[jj] / h[jj] for jj in xrange(K))
    c /= 1/z + sum(1.0 / h[jj] for jj in xrange(K))
    for ii in xrange(K):
      new_alpha[ii] = old_alpha[ii] + tau * (grad[ii] - c) / h[ii]

    failure = True
    try:
      new_elbo = l_alpha(new_alpha, M, K, gamma_grad)
    except OverflowError:
      new_elbo = NEGATIVE_INFINITY
    if new_elbo < elbo:
      tau /= 2
      new_alpha = old_alpha
      print "Step failed", new_elbo, elbo
    else:
      elbo = new_elbo
      failure = False

    print "-----------------------------"
    print grad
    print h
    print c
    print new_alpha

    
