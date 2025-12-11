import { defineConfig } from 'astro/config';
import sitemap from '@astrojs/sitemap';

// https://astro.build/config
export default defineConfig({
  // site: 'https://devkiraa.github.io',
  // base: '/nex',
  integrations: [sitemap()],
  output: 'static',
});
