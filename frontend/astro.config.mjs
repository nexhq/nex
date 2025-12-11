import { defineConfig } from 'astro/config';
import sitemap from '@astrojs/sitemap';

// https://astro.build/config
export default defineConfig({
  site: 'https://try-nex.vercel.app',
  // base: '/nex',
  integrations: [sitemap()],
  output: 'static',
});
