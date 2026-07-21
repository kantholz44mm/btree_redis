source("../common.R")

# r <- read_broken_csv('vary1.csv.gz')
r <- bind_rows(
  # python3 R/size3/leaf-size-density.py |parallel -j8 --joblog joblog -- {1}| tee R/size3/leaf-size-density.csv
  read_broken_csv('leaf-size-density.csv.gz') |> mutate(file = 3)
)

d <- r |> augment()

# ---- Plot 1 ----
p1 <- d |> 
  ggplot() +
  geom_line(
    aes(density, nodeCount_Dense / leaf_count, col = factor(psl)),
    stat = "summary",
    fun = mean
  ) +
  scale_color_hue()

save_plot_obj("leaf_density_dense_ratio", p1, h = 60)

# ---- Plot 2 ----
p2 <- d |> 
  ggplot() +
  geom_line(
    aes(density, leaf_count * 2^psl, col = factor(psl)),
    stat = "summary",
    fun = mean
  ) +
  scale_color_hue()

save_plot_obj("leaf_density_scaled_leafcount", p2, h = 60)
